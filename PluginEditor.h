#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class CustomVisualizer : public juce::Component, private juce::Timer
{
public:
    CustomVisualizer(VibeshifterProcessor& p);
    ~CustomVisualizer() override;
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;
    VibeshifterProcessor& proc;
    float ballPhase = 0.0f;
};

class VibeshifterEditor : public juce::AudioProcessorEditor
{
public:
    VibeshifterEditor(VibeshifterProcessor&);
    ~VibeshifterEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VibeshifterProcessor& processorRef;
    CustomVisualizer visualizer;

    juce::Slider pitchSlider, formantSlider, mixSlider;
    juce::Slider wowRateSlider, wowDepthSlider;
    juce::ToggleButton wowSyncButton;
    juce::ComboBox satModeBox;
    juce::Slider satDriveSlider;
    juce::Slider hpfSlider, lpfSlider;
    juce::Slider spreadSlider, chorusRateSlider, chorusDepthSlider;
    juce::Slider delayTimeSlider, delayFbSlider, delayMixSlider;

    juce::Label pitchLabel, formantLabel, mixLabel;
    juce::Label wowRateLabel, wowDepthLabel;
    juce::Label satDriveLabel, hpfLabel, lpfLabel;
    juce::Label spreadLabel, chorusRateLabel, chorusDepthLabel;
    juce::Label delayTimeLabel, delayFbLabel, delayMixLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAttach, formantAttach, mixAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wowRateAttach, wowDepthAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> wowSyncAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> satModeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> satDriveAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hpfAttach, lpfAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> spreadAttach, chorusRateAttach, chorusDepthAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttach, delayFbAttach, delayMixAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VibeshifterEditor)
};
