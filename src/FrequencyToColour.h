#pragma once
#include <JuceHeader.h>

class FrequencyToColour
{
public:
    bool naturalDirection = false;  // false = low→red, high→violet

    juce::Colour getColourForFrequencyInRange (float frequencyHz,
                                               float minFrequencyHz,
                                               float maxFrequencyHz) const
    {
        frequencyHz = juce::jlimit (minFrequencyHz, maxFrequencyHz, frequencyHz);

        const float logMin = std::log10 (minFrequencyHz);
        const float logMax = std::log10 (maxFrequencyHz);
        float t = (std::log10 (frequencyHz) - logMin) / (logMax - logMin);

        const float wlMin = 380.0f;
        const float wlMax = 700.0f;
        float wl = naturalDirection
                   ? wlMin + t * (wlMax - wlMin)
                   : wlMax - t * (wlMax - wlMin);

        return wavelengthToRGB (wl);
    }

    // Returns a full-brightness colour for the given frequency
    juce::Colour getColourForFrequency (float frequencyHz) const
    {
        return getColourForFrequencyInRange (frequencyHz, 20.0f, 20000.0f);
    }

    // Convenience: returns colour scaled by brightness (0=black, 1=full colour)
    juce::Colour getColourForFrequency (float frequencyHz, float brightness) const
    {
        juce::Colour c = getColourForFrequency (frequencyHz);
        float v = juce::jlimit (0.0f, 1.0f, brightness);
        return juce::Colour::fromHSV (c.getHue(), c.getSaturation(), v, 1.0f);
    }

private:
    static juce::Colour wavelengthToRGB (float wl)
    {
        float r = 0.0f, g = 0.0f, b = 0.0f;

        if      (wl >= 380.0f && wl < 440.0f) { r = -(wl - 440.0f) / 60.0f; g = 0.0f; b = 1.0f; }
        else if (wl >= 440.0f && wl < 490.0f) { r = 0.0f; g = (wl - 440.0f) / 50.0f; b = 1.0f; }
        else if (wl >= 490.0f && wl < 510.0f) { r = 0.0f; g = 1.0f; b = -(wl - 510.0f) / 20.0f; }
        else if (wl >= 510.0f && wl < 580.0f) { r = (wl - 510.0f) / 70.0f; g = 1.0f; b = 0.0f; }
        else if (wl >= 580.0f && wl < 645.0f) { r = 1.0f; g = -(wl - 645.0f) / 65.0f; b = 0.0f; }
        else if (wl >= 645.0f && wl <= 700.0f){ r = 1.0f; g = 0.0f; b = 0.0f; }

        // Clamp and return full brightness — no dimming here
        r = juce::jlimit (0.0f, 1.0f, r);
        g = juce::jlimit (0.0f, 1.0f, g);
        b = juce::jlimit (0.0f, 1.0f, b);

        return juce::Colour::fromFloatRGBA (r, g, b, 1.0f);
    }
};
