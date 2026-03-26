#pragma once
#include <JuceHeader.h>
#include <array>

/**
 * SpectrumAnalyser
 *
 * 8192-point FFT with 75% overlap (hop = fftSize/4) so new frames fire
 * ~4x more often — at 44100Hz that's ~46ms per frame instead of ~186ms.
 * This gives smooth waterfall motion without sacrificing frequency resolution.
 */
class SpectrumAnalyser
{
public:
    static constexpr int fftOrder = 13;           // 2^13 = 8192 point FFT
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int numBins  = fftSize / 2;

    // Hop size = how many new samples before we fire the FFT
    // fftSize/4 = 75% overlap = ~4x more frames per second
    static constexpr int hopSize  = fftSize / 4;

    // ── Tunable smoothing ─────────────────────────────────────────────────
    // Range 0..1 — lower = faster response, higher = smoother/slower
    float attack = 0.5f;   // how fast magnitudes rise
    float decay  = 0.65f;  // how fast magnitudes fall

    SpectrumAnalyser()
        : forwardFFT (fftOrder),
          window     (fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        fftData.fill (0.0f);
        smoothedData.fill (0.0f);
        fifo.fill (0.0f);
    }

    // Call from audio thread
    void pushSample (float sample)
    {
        // Ring buffer — always keep the last fftSize samples
        fifo[static_cast<size_t>(writePos)] = sample;
        writePos = (writePos + 1) % fftSize;
        hopCounter++;

        if (hopCounter >= hopSize)
        {
            hopCounter = 0;
            if (!nextFFTBlockReady)
            {
                // Copy ring buffer into fftData in order
                for (int i = 0; i < fftSize; ++i)
                    fftData[static_cast<size_t>(i)] = fifo[static_cast<size_t>((writePos + i) % fftSize)];
                nextFFTBlockReady = true;
            }
        }
    }

    // Call from GUI timer — returns true if a new frame was processed
    bool drawNextFrameIfAvailable()
    {
        if (!nextFFTBlockReady)
            return false;

        // Work on a local copy so audio thread can keep writing
        std::array<float, fftSize * 2> workBuf;
        std::copy (fftData.begin(), fftData.begin() + fftSize, workBuf.begin());
        std::fill (workBuf.begin() + fftSize, workBuf.end(), 0.0f);
        nextFFTBlockReady = false;

        window.multiplyWithWindowingTable (workBuf.data(), fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform (workBuf.data());

        for (int i = 0; i < numBins; ++i)
        {
            float magnitude  = workBuf[static_cast<size_t>(i)] / (float) fftSize;
            float db         = juce::Decibels::gainToDecibels (magnitude, -100.0f);
            float normalised = juce::jlimit (0.0f, 1.0f,
                                juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f));

            float coeff = (normalised > smoothedData[static_cast<size_t>(i)]) ? attack : decay;
            smoothedData[static_cast<size_t>(i)] = coeff * smoothedData[static_cast<size_t>(i)] + (1.0f - coeff) * normalised;
        }

        return true;
    }

    float getMagnitude (int bin) const
    {
        jassert (bin >= 0 && bin < numBins);
        return smoothedData[static_cast<size_t>(bin)];
    }

    static float binToFrequency (int bin, double sampleRate)
    {
        return (float) bin * (float) sampleRate / (float) fftSize;
    }

private:
    juce::dsp::FFT                      forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, fftSize * 2>      fftData;
    std::array<float, numBins>          smoothedData;
    std::array<float, fftSize>          fifo;

    int  writePos          = 0;
    int  hopCounter        = 0;
    bool nextFFTBlockReady = false;
};
