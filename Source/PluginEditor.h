#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class ChromeCadillacLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;
};

class CadillacShifterAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit CadillacShifterAudioProcessorEditor(CadillacShifterAudioProcessor&);
    ~CadillacShifterAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    CadillacShifterAudioProcessor& audioProcessor;
    ChromeCadillacLookAndFeel chromeLookAndFeel;

    juce::Slider pitchSlider;
    juce::Slider formantSlider;
    juce::Slider shifterMixSlider;
    juce::Slider chorusMixSlider;

    juce::Label pitchLabel;
    juce::Label formantLabel;
    juce::Label shifterMixLabel;
    juce::Label chorusMixLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> pitchAttachment;
    std::unique_ptr<Attachment> formantAttachment;
    std::unique_ptr<Attachment> shifterMixAttachment;
    std::unique_ptr<Attachment> chorusMixAttachment;

    void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& title, const juce::String& suffix);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CadillacShifterAudioProcessorEditor)
};
