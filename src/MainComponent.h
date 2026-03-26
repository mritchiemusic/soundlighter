#pragma once
#include <JuceHeader.h>
#include "SpectrumAnalyser.h"
#include "FrequencyToColour.h"

//==============================================================================
class GearButton : public juce::Button
{
public:
    GearButton() : juce::Button ("Settings") {}

    void paintButton (juce::Graphics& g, bool isHighlighted, bool) override
    {
        auto area = getLocalBounds().toFloat().reduced (6.0f);
        float cx = area.getCentreX(), cy = area.getCentreY(), r = area.getWidth() * 0.5f;
        float alpha = isHighlighted ? 0.9f : 0.6f;

        if (showClose)
        {
            g.setColour (juce::Colours::white.withAlpha (alpha));
            auto b = getLocalBounds().toFloat().reduced (10.0f);
            g.drawLine (b.getX(), b.getY(), b.getRight(), b.getBottom(), 2.0f);
            g.drawLine (b.getRight(), b.getY(), b.getX(), b.getBottom(), 2.0f);
            return;
        }

        const int   numTeeth  = 8;
        const float outerR    = r * 0.72f;
        const float innerR    = r * 0.44f;

        juce::Path p;
        for (int i = 0; i < numTeeth * 2; ++i)
        {
            float angle = (float) i * juce::MathConstants<float>::pi / (float) numTeeth
                          - juce::MathConstants<float>::halfPi;
            float rad = (i % 2 == 0) ? outerR : innerR;
            float x = cx + rad * std::cos (angle);
            float y = cy + rad * std::sin (angle);
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        p.closeSubPath();
        p.addEllipse (cx - r * 0.22f, cy - r * 0.22f, r * 0.44f, r * 0.44f);

        g.setColour (juce::Colours::white.withAlpha (alpha));
        g.fillPath (p);
    }

    bool showClose = false;
};

//==============================================================================
class MainComponent : public juce::AudioAppComponent,
                      public juce::Timer,
                      public juce::ChangeListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo&) override;
    void releaseResources() override;

    void paint    (juce::Graphics&) override;
    void resized  () override;
    void timerCallback () override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    void drawColourRect     (juce::Graphics&, juce::Rectangle<int>);
    void drawWaterfall      (juce::Graphics&, juce::Rectangle<int>);
    void drawFrequencyLabels(juce::Graphics&, juce::Rectangle<int>);
    void buildDeviceSelector();
    void layoutSettings();

    // Adjustable display parameters
    float minBoost = 0.18f;
    float silenceThreshold = 0.003f;

    // Sliders for display parameters
    juce::Slider minBoostSlider;
    juce::Slider silenceThresholdSlider;
    juce::Label minBoostLabel;
    juce::Label silenceThresholdLabel;

    int  dividerY()       const { return (int)(getHeight() * splitRatio); }
    bool nearDivider(int y) const { return std::abs (y - dividerY()) < 6; }

    // Audio
    SpectrumAnalyser  analyser;
    FrequencyToColour colourMapper;
    double currentSampleRate = 44100.0;

    // Display
    float smoothedFundFreq  = 440.0f;
    float smoothedAmplitude = 0.0f;
    juce::Colour displayColour { juce::Colours::black };

    // Waterfall — colour holds hue, alpha holds magnitude
    static constexpr int waterfallRows = 120;
    std::array<std::array<juce::Colour, SpectrumAnalyser::numBins>, waterfallRows> waterfall;
    // int waterfallHead = 0; // Removed unused field

    // Tone generator
    bool  toneEnabled        = false;
    float toneFrequency      = 440.0f;
    float toneAmplitude      = 0.3f;
    float tonePhase          = 0.0f;
    float tonePhaseIncrement = 0.0f;

    // Layout
    float splitRatio      = 0.45f;
    bool  draggingDivider = false;

    // Settings UI
    bool       settingsVisible = false;
    GearButton settingsButton;

    juce::Component    settingsPanel;
    juce::Label        deviceLabel, freqLabel, freqValueLabel, volLabel, attackLabel, decayLabel;
    juce::ComboBox     inputDeviceBox;
    juce::TextButton   refreshButton { "↺" };
    juce::ToggleButton toneToggle    { "Tone generator" };
    juce::Slider       freqSlider, volSlider, attackSlider, decaySlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
