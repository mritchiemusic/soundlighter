#include "MainComponent.h"

MainComponent::MainComponent()
{
    // Setup minBoostSlider
    minBoostSlider.setRange(0.0, 1.0, 0.01);
    minBoostSlider.setValue(minBoost);
    minBoostSlider.onValueChange = [this] {
        minBoost = (float)minBoostSlider.getValue();
    };

    // Setup silenceThresholdSlider
    silenceThresholdSlider.setRange(0.0, 0.05, 0.0001);
    silenceThresholdSlider.setValue(silenceThreshold);
    silenceThresholdSlider.onValueChange = [this] {
        silenceThreshold = (float)silenceThresholdSlider.getValue();
    };
    minBoostLabel.setText("Min Boost", juce::dontSendNotification);
    minBoostLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    settingsPanel.addAndMakeVisible(minBoostLabel);
    silenceThresholdLabel.setText("Silence Threshold", juce::dontSendNotification);
    silenceThresholdLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    settingsPanel.addAndMakeVisible(silenceThresholdLabel);
    // ...existing code...

    // ...existing code...

    // ...existing code...
        // ...existing code...
        auto setupSlider = [](juce::Slider& s, juce::Component& parent)
        {
            s.setSliderStyle (juce::Slider::LinearHorizontal);
            s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            parent.addAndMakeVisible (s);
        };

        setupSlider(minBoostSlider, settingsPanel);
        setupSlider(silenceThresholdSlider, settingsPanel);
    colourMapper.naturalDirection = false;

    for (auto& row : waterfall)
        row.fill (juce::Colours::black);

    settingsButton.onClick = [this]
    {
        settingsVisible = !settingsVisible;
        settingsButton.showClose = settingsVisible;
        settingsPanel.setVisible (settingsVisible);
        layoutSettings();
        repaint();
    };
    addAndMakeVisible (settingsButton);

    settingsPanel.setVisible (false);
    addAndMakeVisible (settingsPanel);

    auto setupLabel = [](juce::Label& l, const juce::String& text, juce::Component& parent)
    {
        l.setText (text, juce::dontSendNotification);
        l.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        parent.addAndMakeVisible (l);
    };

    setupLabel (deviceLabel, "Input device:", settingsPanel);
    setupLabel (freqLabel,   "Frequency",     settingsPanel);
    setupLabel (volLabel,    "Volume",        settingsPanel);
    setupLabel (attackLabel, "Attack",        settingsPanel);
    setupLabel (decayLabel,  "Decay",         settingsPanel);

    freqValueLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    freqValueLabel.setJustificationType (juce::Justification::centredRight);
    freqValueLabel.setText ("440 Hz", juce::dontSendNotification);
    settingsPanel.addAndMakeVisible (freqValueLabel);

    inputDeviceBox.setTextWhenNothingSelected ("Select input...");
    settingsPanel.addAndMakeVisible (inputDeviceBox);

    refreshButton.onClick = [this] { buildDeviceSelector(); };
    settingsPanel.addAndMakeVisible (refreshButton);

    toneToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    toneToggle.onClick = [this]
    {
        toneEnabled = toneToggle.getToggleState();
        freqSlider.setEnabled (toneEnabled);
        volSlider.setEnabled  (toneEnabled);
    };
    settingsPanel.addAndMakeVisible (toneToggle);



    freqSlider.setRange (20.0, 20000.0, 1.0);
    freqSlider.setSkewFactorFromMidPoint (1000.0);
    freqSlider.setValue (440.0);
    freqSlider.setEnabled (false);
    setupSlider (freqSlider, settingsPanel);
    freqSlider.onValueChange = [this]
    {
        toneFrequency = (float) freqSlider.getValue();
        tonePhaseIncrement = juce::MathConstants<float>::twoPi
                             * toneFrequency / (float) currentSampleRate;
        freqValueLabel.setText (juce::String ((int) toneFrequency) + " Hz",
                                juce::dontSendNotification);
    };

    volSlider.setRange (0.0, 1.0, 0.01);
    volSlider.setValue (0.3);
    volSlider.setEnabled (false);
    setupSlider (volSlider, settingsPanel);
    volSlider.onValueChange = [this] { toneAmplitude = (float) volSlider.getValue(); };

    attackSlider.setRange (0.0, 0.99, 0.01);
    attackSlider.setValue (analyser.attack);
    setupSlider (attackSlider, settingsPanel);
    attackSlider.onValueChange = [this] { analyser.attack = (float) attackSlider.getValue(); };

    decaySlider.setRange (0.0, 0.99, 0.01);
    decaySlider.setValue (analyser.decay);
    setupSlider (decaySlider, settingsPanel);
    decaySlider.onValueChange = [this] { analyser.decay = (float) decaySlider.getValue(); };

    setSize (1100, 760);
    setAudioChannels (2, 2);
    buildDeviceSelector();
    deviceManager.addChangeListener (this);
    startTimerHz (120);
}

MainComponent::~MainComponent()
{
    stopTimer();
    deviceManager.removeChangeListener (this);
    shutdownAudio();
}

//==============================================================================
void MainComponent::layoutSettings()
{
    if (!settingsVisible) { settingsPanel.setBounds (0, 0, 0, 0); return; }

    const int pw = 280, ph = 400;
    settingsPanel.setBounds (getWidth() - pw - 8, 48, pw, ph);

    auto b = settingsPanel.getLocalBounds().reduced (12);

    auto deviceRow = b.removeFromTop (28);
    deviceLabel.setBounds (deviceRow.removeFromLeft (90));
    refreshButton.setBounds (deviceRow.removeFromRight (28));
    deviceRow.removeFromRight (4);
    inputDeviceBox.setBounds (deviceRow);

    b.removeFromTop (10);
    toneToggle.setBounds (b.removeFromTop (24));
    b.removeFromTop (8);

    auto freqRow = b.removeFromTop (20);
    freqLabel.setBounds (freqRow.removeFromLeft (70));
    freqValueLabel.setBounds (freqRow.removeFromRight (56));
    freqSlider.setBounds (freqRow);

    b.removeFromTop (6);
    auto volRow = b.removeFromTop (20);
    volLabel.setBounds (volRow.removeFromLeft (70));
    volSlider.setBounds (volRow);

    b.removeFromTop (14);

    auto attackRow = b.removeFromTop (20);
    attackLabel.setBounds (attackRow.removeFromLeft (70));
    attackSlider.setBounds (attackRow);

    b.removeFromTop (6);
    auto decayRow = b.removeFromTop (20);
    decayLabel.setBounds (decayRow.removeFromLeft (70));
    decaySlider.setBounds (decayRow);

    b.removeFromTop (10);

    // Move minBoost and silenceThreshold controls right after decay
    auto minBoostRow = b.removeFromTop (20);
    minBoostLabel.setBounds (minBoostRow.removeFromLeft (110));
    minBoostSlider.setBounds (minBoostRow);

    b.removeFromTop (6);
    auto silenceRow = b.removeFromTop (20);
    silenceThresholdLabel.setBounds (silenceRow.removeFromLeft (110));
    silenceThresholdSlider.setBounds (silenceRow);

    // ...existing code for any controls that follow...
}

void MainComponent::buildDeviceSelector()
{
    inputDeviceBox.clear();
    auto& deviceType = *deviceManager.getCurrentDeviceTypeObject();
    auto  deviceNames = deviceType.getDeviceNames (true);
    for (int i = 0; i < deviceNames.size(); ++i)
        inputDeviceBox.addItem (deviceNames[i], i + 1);
    if (auto* dev = deviceManager.getCurrentAudioDevice())
        for (int i = 0; i < deviceNames.size(); ++i)
            if (deviceNames[i] == dev->getName())
                inputDeviceBox.setSelectedId (i + 1, juce::dontSendNotification);

    inputDeviceBox.onChange = [this]
    {
        auto& dt    = *deviceManager.getCurrentDeviceTypeObject();
        auto  deviceNames = dt.getDeviceNames (true);
        int   idx   = inputDeviceBox.getSelectedId() - 1;
        if (idx < 0 || idx >= deviceNames.size()) return;
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup (setup);
        setup.inputDeviceName         = deviceNames[idx];
        setup.useDefaultInputChannels = false;
        setup.inputChannels.setRange (0, 32, false);
        setup.inputChannels.setRange (0, 2,  true);
        deviceManager.setAudioDeviceSetup (setup, true);
    };
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*) { buildDeviceSelector(); }

//==============================================================================
void MainComponent::prepareToPlay (int, double sampleRate)
{
    currentSampleRate  = sampleRate;
    tonePhaseIncrement = juce::MathConstants<float>::twoPi * toneFrequency / (float) sampleRate;
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* buffer    = bufferToFill.buffer;
    const int start = bufferToFill.startSample;
    const int n     = bufferToFill.numSamples;
    const int numCh = buffer->getNumChannels();

    for (int i = 0; i < n; ++i)
    {
        float sum = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            sum += buffer->getReadPointer (ch, start)[i];
        if (numCh > 0) sum /= (float) numCh;
        analyser.pushSample (sum);
    }

    if (toneEnabled)
    {
        for (int i = 0; i < n; ++i)
        {
            float s = toneAmplitude * std::sin (tonePhase);
            tonePhase += tonePhaseIncrement;
            if (tonePhase >= juce::MathConstants<float>::twoPi)
                tonePhase -= juce::MathConstants<float>::twoPi;
            analyser.pushSample (s);
            for (int ch = 0; ch < numCh; ++ch)
                buffer->setSample (ch, start + i, s);
        }
    }
    else
    {
        for (int ch = 0; ch < numCh; ++ch)
            buffer->clear (ch, start, n);
    }
}

void MainComponent::releaseResources() {}

//==============================================================================
void MainComponent::timerCallback()
{
    if (!analyser.drawNextFrameIfAvailable())
        return;

    // Peak bin for fundamental frequency
    float peak = 0.0f;
    int   peakBin = 1;
    float rms = 0.0f;
    for (int b = 1; b < SpectrumAnalyser::numBins; ++b)
    {
        float m = analyser.getMagnitude (b);
        rms += m * m;
        if (m > peak) { peak = m; peakBin = b; }
    }
    rms = std::sqrt (rms / (float) SpectrumAnalyser::numBins);

    float fundFreq = juce::jlimit (20.0f, 20000.0f,
                        SpectrumAnalyser::binToFrequency (peakBin, currentSampleRate));
    if (peak > 0.05f)
        smoothedFundFreq = smoothedFundFreq * 0.4f + fundFreq * 0.6f;

    smoothedAmplitude = juce::jlimit (0.0f, 1.0f,
                            smoothedAmplitude * 0.5f + rms * 0.5f * 8.0f);

    // Top colour rect — full hue, brightness from amplitude
    float bri = std::pow (smoothedAmplitude, 0.33f);
    juce::Colour baseCol = colourMapper.getColourForFrequency (smoothedFundFreq);
    // Scale raw RGB directly — fromHSV broken on this platform
    juce::Colour target = juce::Colour (
        (uint8_t)(baseCol.getRed()   * bri),
        (uint8_t)(baseCol.getGreen() * bri),
        (uint8_t)(baseCol.getBlue()  * bri));
    displayColour = displayColour.interpolatedWith (target, 0.12f);

    repaint();
}

//==============================================================================
void MainComponent::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (nearDivider (e.y) ? juce::MouseCursor::UpDownResizeCursor
                                      : juce::MouseCursor::NormalCursor);
}

void MainComponent::mouseDown (const juce::MouseEvent& e)
{
    draggingDivider = nearDivider (e.y);
}

void MainComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (!draggingDivider) return;
    splitRatio = juce::jlimit (0.1f, 0.9f, (float) e.y / (float) getHeight());
    resized();
    repaint();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    const int div    = dividerY();
    const int labelH = 22;

    drawColourRect      (g, { 0, 0, getWidth(), div });
    drawWaterfall       (g, { 0, div, getWidth(), getHeight() - div - labelH });
    drawFrequencyLabels (g, { 0, getHeight() - labelH, getWidth(), labelH });

    if (settingsVisible)
    {
        auto pb = settingsPanel.getBounds();
        g.setColour (juce::Colour (0xee111118));
        g.fillRoundedRectangle (pb.toFloat(), 8.0f);
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawRoundedRectangle (pb.toFloat(), 8.0f, 1.0f);
    }
}

void MainComponent::drawColourRect (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (displayColour);
    g.fillRect (area);

    if (smoothedAmplitude > 0.03f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.setFont (13.0f);
        juce::String txt = smoothedFundFreq < 1000.0f
            ? juce::String ((int) smoothedFundFreq) + " Hz"
            : juce::String (smoothedFundFreq / 1000.0f, 2) + " kHz";
        g.drawText (txt, area.reduced (12), juce::Justification::bottomLeft);
    }
}

void MainComponent::drawWaterfall (juce::Graphics& g, juce::Rectangle<int> area)
{
    if (area.getHeight() < 2) return;

    const int imgW = area.getWidth();
    const int imgH = area.getHeight();
    const float logMin = std::log10 (20.0f);
    const float logMax = std::log10 (20000.0f);

    juce::Image img (juce::Image::ARGB, imgW, imgH, true);

    // For each horizontal pixel, map to a frequency bin and draw a vertical stripe
    float minBoostVal = minBoost;
    float silenceThresholdVal = silenceThreshold;
    for (int px = 0; px < imgW; ++px)
    {
        float t = (float) px / (float) imgW;
        float freq = std::pow (10.0f, logMin + t * (logMax - logMin));
        float exactBin = freq * (float) SpectrumAnalyser::fftSize / (float) currentSampleRate;
        int binLo = juce::jlimit (0, SpectrumAnalyser::numBins - 2, (int) exactBin);
        float frac = exactBin - (float) binLo;

        float magLo = analyser.getMagnitude (binLo);
        float magHi = analyser.getMagnitude (binLo + 1);
        float mag = magLo + frac * (magHi - magLo);

        float finalBrightness = 0.0f;
        if (mag > silenceThresholdVal)
        {
            float boostedMag = juce::jlimit (minBoostVal, 1.0f, mag * (1.0f - minBoostVal) + minBoostVal);
            finalBrightness = juce::jlimit (0.0f, 1.0f, boostedMag * 4.0f);
        }

        juce::Colour cLo = colourMapper.getColourForFrequency (
            SpectrumAnalyser::binToFrequency (binLo, currentSampleRate));
        juce::Colour cHi = colourMapper.getColourForFrequency (
            SpectrumAnalyser::binToFrequency (binLo + 1, currentSampleRate));
        juce::Colour c = cLo.interpolatedWith (cHi, frac);

        juce::Colour finalCol (
            (uint8_t)(c.getRed()   * finalBrightness),
            (uint8_t)(c.getGreen() * finalBrightness),
            (uint8_t)(c.getBlue()  * finalBrightness));

        for (int py = 0; py < imgH; ++py)
            img.setPixelAt (px, py, finalCol);
    }

    g.drawImage (img, static_cast<int>(area.getX()), static_cast<int>(area.getY()),
                 static_cast<int>(area.getWidth()), static_cast<int>(area.getHeight()),
                 0, 0, imgW, imgH, false);
}

void MainComponent::drawFrequencyLabels (juce::Graphics& g, juce::Rectangle<int> area)
{
    const float logMin = std::log10 (20.0f);
    const float logMax = std::log10 (20000.0f);
    const float freqs[]  = { 20,50,100,200,500,1000,2000,5000,10000,20000 };
    const char* labels[] = { "20","50","100","200","500","1k","2k","5k","10k","20k" };

    g.setFont (10.0f);
    for (int i = 0; i < 10; ++i)
    {
        float t = (std::log10 (freqs[i]) - logMin) / (logMax - logMin);
        int   x = (int) (t * getWidth());
        juce::Colour c = colourMapper.getColourForFrequency (freqs[i]);
        g.setColour (c.withMultipliedBrightness (1.4f));
        g.drawText (juce::String (labels[i]) + " Hz", x - 20, area.getY(), 40,
                    area.getHeight(), juce::Justification::centred);
    }
}

//==============================================================================
void MainComponent::resized()
{
    settingsButton.setBounds (getWidth() - 40, 8, 32, 32);
    layoutSettings();
}
