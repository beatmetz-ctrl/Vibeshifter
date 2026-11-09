#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class StereoVocalShifterAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    StereoVocalShifterAudioProcessorEditor(StereoVocalShifterAudioProcessor&);
    ~StereoVocalShifterAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    StereoVocalShifterAudioProcessor& audioProcessor;

    juce::Slider pitchSlider;
    juce::Slider formantSlider;
    juce::Slider mixSlider;

    juce::Label pitchLabel;
    juce::Label formantLabel;
    juce::Label mixLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> formantAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoVocalShifterAudioProcessorEditor)
};
