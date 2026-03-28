#include "Test_Audio_Engine.h"

Test_Audio_Engine::Test_Audio_Engine()
{
    engine = std::make_unique<AudioEngine>();
    engine->start();

    addAndMakeVisible(recordButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(monitorButton);
    addAndMakeVisible(inputGainKnob);
    addAndMakeVisible(outputGainKnob);

    recordButton.onClick = [this]
    {
        engine->startRecording();
    };

    stopButton.onClick = [this]
    {
        engine->stopRecording();
        engine->stopPlayback();
    };

    playButton.onClick = [this]
    {
        engine->startPlayback();
    };

    monitorButton.onClick = [this]
    {
        monitoringOn = !monitoringOn;
        engine->setMonitoringEnabled(monitoringOn);
        monitorButton.setButtonText(monitoringOn ? "Monitor: ON" : "Monitor: OFF");
    };

   
    inputGainKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    inputGainKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    inputGainKnob.setRange(0.0, 2.0, 0.01);
    inputGainKnob.setValue(1.0);

    inputGainKnob.onValueChange = [this]
    {
        engine->setInputGain((float)inputGainKnob.getValue());
    };

 
    outputGainKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    outputGainKnob.setRange(0.0, 2.0, 0.01);
    outputGainKnob.setValue(1.0);

    outputGainKnob.onValueChange = [this]
    {
        engine->setOutputGain((float)outputGainKnob.getValue());
    };

    startTimerHz(30);
    setSize(700, 450);
}

Test_Audio_Engine::~Test_Audio_Engine()
{
    if (engine)
        engine->stop();
}

void Test_Audio_Engine::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawFittedText("Audio Engine Test",
                     getLocalBounds().removeFromTop(40),
                     juce::Justification::centred,
                     1);

    g.setColour(juce::Colours::green);
    int meterWidth = (int)(getWidth() * lastPeak);
    g.fillRect(0, getHeight() - 40, meterWidth, 40);
}

void Test_Audio_Engine::resized()
{
    auto area = getLocalBounds().reduced(20);

    area.removeFromTop(60);
    area.removeFromTop(10);

    auto buttonRow = area.removeFromTop(40);

    int totalButtonWidth = 4 * 120;
    int gap = (buttonRow.getWidth() - totalButtonWidth) / 2;

    buttonRow.removeFromLeft(gap);

    recordButton.setBounds(buttonRow.removeFromLeft(120));
    stopButton.setBounds(buttonRow.removeFromLeft(120));
    playButton.setBounds(buttonRow.removeFromLeft(120));
    monitorButton.setBounds(buttonRow.removeFromLeft(120));

    auto knobRow = area.removeFromTop(180);
    inputGainKnob.setBounds(knobRow.removeFromLeft(knobRow.getWidth() / 2).withSizeKeepingCentre(140, 140));
    outputGainKnob.setBounds(knobRow.withSizeKeepingCentre(140, 140));
}

void Test_Audio_Engine::timerCallback()
{
    const auto& buffer = engine->getLatestInput();

    if (buffer.getNumSamples() > 0)
    {
        float rawPeak = buffer.getMagnitude(0, 0, buffer.getNumSamples());

        if (rawPeak > smoothedPeak)
            smoothedPeak = attack * smoothedPeak + (1.0f - attack) * rawPeak;
        else
            smoothedPeak = release * smoothedPeak + (1.0f - release) * rawPeak;

        lastPeak = smoothedPeak;
    }

    repaint();
}