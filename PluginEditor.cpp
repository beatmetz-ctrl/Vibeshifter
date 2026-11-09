#include "PluginProcessor.h"
#include "PluginEditor.h"

StereoVocalShifterAudioProcessorEditor::StereoVocalShifterAudioProcessorEditor(StereoVocalShifterAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    auto setupFader = [this](juce::Slider& slider, juce::Label& label, const juce::String& text, const juce::String& suffix)
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 24);
        slider.setTextValueSuffix(suffix);
        slider.setColour(juce::Slider::trackColourId, juce::Colour(0xff00d2ff));
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2c2c34));
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(14.0f, juce::Font::bold));
        label.setColour(juce::Colours::lightgrey);
        addAndMakeVisible(label);
    };

    setupFader(pitchSlider, pitchLabel, "PITCH", " st");
    pitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "PITCH", pitchSlider);

    setupFader(formantSlider, formantLabel, "FORMANT", " st");
    formantAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "FORMANT", formantSlider);

    setupFader(mixSlider, mixLabel, "DRY / WET", " %");
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MIX", mixSlider);

    setSize(480, 260);
}

void StereoVocalShifterAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff18181c));

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("STEREO VOCAL SHIFTER", 20, 15, getWidth() - 40, 26, juce::Justification::centredLeft);

    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(12.0f));
    g.drawText("MeatTheBeat.de", 20, getHeight() - 26, getWidth() - 40, 20, juce::Justification::centredRight);
}

void StereoVocalShifterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(35);    // Platz für Titel
    area.removeFromBottom(20); // Platz für Footer

    int rowHeight = area.getHeight() / 3;

    auto placeRow = [](juce::Rectangle<int> rowBounds, juce::Label& label, juce::Slider& slider)
    {
        label.setBounds(rowBounds.removeFromTop(20));
        slider.setBounds(rowBounds.reduced(0, 4));
    };

    placeRow(area.removeFromTop(rowHeight), pitchLabel, pitchSlider);
    placeRow(area.removeFromTop(rowHeight), formantLabel, formantSlider);
    placeRow(area.removeFromTop(rowHeight), mixLabel, mixSlider);
}
