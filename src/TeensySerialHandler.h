#pragma once

#include <JuceHeader.h>
#include "SpectrumRingBuffer.h"
#include <array>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <glob.h>
#include <cerrno>

/**
 * USB Serial handler for Teensy LED controller (macOS/POSIX implementation).
 * Sends spectrum frames over serial @ 115200 baud.
 * Uses a background thread to avoid blocking audio.
 */
class TeensySerialHandler : public juce::Thread
{
public:
    TeensySerialHandler(SpectrumRingBuffer& buffer)
        : juce::Thread("TeensySerialHandler"), spectrumBuffer_(buffer), fd_(-1)
    {
    }

    ~TeensySerialHandler() override
    {
        stopThread(2000);
        closePort();
    }

    /**
     * Scan for Teensy serial ports and auto-connect.
     * Returns true if successfully opened.
     */
    bool openPort()
    {
        // Search for Teensy devices on macOS: /dev/cu.usbmodem*
        // Use cu.* for outbound writes from host apps.
        glob_t glob_result;
        glob("/dev/cu.usbmodem*", 0, nullptr, &glob_result);

        bool opened = false;
        for (size_t i = 0; i < glob_result.gl_pathc; ++i)
        {
            const char* device = glob_result.gl_pathv[i];
            if (openDevice(device))
            {
                DBG("Connected to Teensy on: " << device);
                opened = true;
                startThread();
                break;
            }
        }

        globfree(&glob_result);
        return opened;
    }

    void closePort()
    {
        if (fd_ >= 0)
        {
            close(fd_);
            fd_ = -1;
        }
    }

    bool isConnected() const
    {
        return fd_ >= 0;
    }

    int getFramesSent() const { return framesSent_; }
    int getFramesDropped() const { return framesDropped_; }

private:
    // Serial protocol constants (match Teensy firmware)
    static constexpr uint8_t START_BYTE = 0xAA;
    static constexpr uint8_t END_BYTE = 0x55;
    static constexpr uint16_t NUM_LEDS = 144;

    SpectrumRingBuffer& spectrumBuffer_;
    int fd_;
    int framesSent_ = 0;
    int framesDropped_ = 0;

    /**
     * Open and configure a serial device @ 115200 baud
     */
    bool openDevice(const char* device)
    {
        fd_ = open(device, O_RDWR | O_NOCTTY);
        if (fd_ < 0)
            return false;

        // Configure terminal settings
        struct termios tty;
        tcgetattr(fd_, &tty);

        // Baud rate
        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);

        // 8 data bits, 1 stop bit, no parity
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;
        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~CSTOPB;

        // Raw mode
        tty.c_lflag &= ~(ICANON | ECHO);
        tty.c_oflag = 0;
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);

        tcsetattr(fd_, TCSANOW, &tty);
        return true;
    }

    uint8_t calculateChecksum(const uint8_t* data, size_t length)
    {
        uint8_t csum = 0;
        for (size_t i = 0; i < length; ++i)
            csum ^= data[i];
        return csum;
    }

    void run() override
    {
        std::array<uint8_t, SpectrumRingBuffer::BYTES_PER_FRAME> frameData;

        while (!threadShouldExit())
        {
            if (fd_ < 0)
            {
                juce::Thread::sleep(100);
                continue;
            }

            // Check for new spectrum data
            if (spectrumBuffer_.readFrame(frameData))
            {
                // Build frame: [START] [LEN_HI] [LEN_LO] [RGB×144] [CSUM] [END]
                std::array<uint8_t, 2 + 2 + SpectrumRingBuffer::BYTES_PER_FRAME + 1 + 1> packet;

                packet[0] = START_BYTE;
                packet[1] = (NUM_LEDS >> 8) & 0xFF;
                packet[2] = NUM_LEDS & 0xFF;

                std::memcpy(&packet[3], frameData.data(), frameData.size());

                uint8_t csum = calculateChecksum(&packet[1], 2 + frameData.size());
                packet[3 + frameData.size()] = csum;
                packet[packet.size() - 1] = END_BYTE;

                // Send packet (write-all with small retry for transient EAGAIN/EINTR)
                size_t totalWritten = 0;
                bool writeOk = true;
                while (totalWritten < packet.size())
                {
                    ssize_t written = write(fd_, packet.data() + totalWritten,
                                            packet.size() - totalWritten);
                    if (written > 0)
                    {
                        totalWritten += (size_t) written;
                        continue;
                    }

                    if (written < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
                    {
                        juce::Thread::sleep(1);
                        continue;
                    }

                    writeOk = false;
                    break;
                }

                if (writeOk && totalWritten == packet.size())
                    framesSent_++;
                else
                    framesDropped_++;
            }
            else
            {
                // No data available; small sleep to avoid busy-wait
                juce::Thread::sleep(1);
            }
        }
    }
};
