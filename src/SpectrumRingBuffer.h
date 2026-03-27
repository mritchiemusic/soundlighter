#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

/**
 * Lock-free circular buffer for spectrum RGB data.
 * Audio thread writes full frames; serial thread reads and transfers.
 * Designed for 144 LEDs (432 bytes per frame).
 */
class SpectrumRingBuffer
{
public:
    static constexpr int NUM_LEDS = 144;
    static constexpr int BYTES_PER_FRAME = NUM_LEDS * 3;  // GRB triplets
    static constexpr int NUM_FRAMES = 4;  // 4 buffered frames
    static constexpr int BUFFER_SIZE = NUM_FRAMES * BYTES_PER_FRAME;

    /**
     * Write a complete frame from audio thread (non-blocking).
     * Overwrites oldest frame if buffer full.
     */
    void writeFrame(const std::array<uint8_t, BYTES_PER_FRAME>& frame)
    {
        int writePos = (writeIndex_.load(std::memory_order_relaxed) % NUM_FRAMES) * BYTES_PER_FRAME;
        std::memcpy(&buffer_[writePos], frame.data(), (size_t)BYTES_PER_FRAME);
        writeIndex_.store(writeIndex_.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }

    /**
     * Check if there's a new frame available for reading.
     */
    bool hasNewFrame() const
    {
        return writeIndex_.load(std::memory_order_acquire) > readIndex_.load(std::memory_order_relaxed);
    }

    /**
     * Read the next available frame (main thread / serial handler).
     * Returns false if no new frame available.
     */
    bool readFrame(std::array<uint8_t, BYTES_PER_FRAME>& frame)
    {
        if (!hasNewFrame())
            return false;

        int readPos = (readIndex_.load(std::memory_order_relaxed) % NUM_FRAMES) * BYTES_PER_FRAME;
        std::memcpy(frame.data(), &buffer_[readPos], (size_t)BYTES_PER_FRAME);
        readIndex_.store(readIndex_.load(std::memory_order_relaxed) + 1, std::memory_order_release);
        return true;
    }

    int getFrameCount() const
    {
        return (int)(writeIndex_.load(std::memory_order_acquire) - readIndex_.load(std::memory_order_acquire));
    }

private:
    std::array<uint8_t, BUFFER_SIZE> buffer_{};
    std::atomic<uint64_t> writeIndex_{0};
    std::atomic<uint64_t> readIndex_{0};
};
