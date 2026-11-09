#include "PluginEditor.h"

CustomVisualizer::CustomVisualizer(VibeshifterProcessor& p) : proc(p)
{
    startTimerHz(30);
}

CustomVisualizer::~CustomVisualizer()
{
    stopTimer();
}

void CustomVisualizer::timerCallback()
{
    repaint();
}

void CustomVisualizer::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    float level = proc.visualizerLevel.load();

    if (level < 0.0005f)
    {
        g.setColour(juce::Colours::darkgrey);
        g.drawRect(getLocalBounds(), 1);
        return;
    }

    float spread = proc.visualizerSpread.load();
    float chorusRate = proc.visualizerChorusRate.load();
    bool delayActive = proc.visualizerDelayActive.load();

    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth();
    float h = bounds.getHeight();
    float centerY = h * 0.5f;

    float lineThickness = juce::jlimit(1.0f, 6.0f, 1.0f + (chorusRate - 1.0f) * 0.5f);
    float bottomSpreadOffset = spread * (h * 0.35f);

    juce::Path lineTop;
    lineTop.startNewSubPath(0.0f, centerY - 10.0f);
    lineTop.lineTo(w, centerY - 10.0f);

    juce::Path lineBottom;
    lineBottom.startNewSubPath(0.0f, centerY + 10.0f);
    lineBottom.lineTo(w * 0.5f, centerY + 10.0f + bottomSpreadOffset);
    lineBottom.lineTo(w, centerY + 10.0f);

    g.setColour(juce::Colours::cyan);
    g.strokePath(lineTop, juce::PathStrokeType(lineThickness));
    g.strokePath(lineBottom, juce::PathStrokeType(lineThickness));

    if (delayActive)
    {
        ballPhase += 0.08f;
        float normPos = std::abs(std::sin(ballPhase));
        float leftBallX = (w * 0.5f) - (normPos * (w * 0.45f));
        float rightBallX = (w * 0.5f) + (normPos * (w * 0.45f));

        g.setColour(juce::Colours::magenta.withAlpha(1.0f - normPos * 0.5f));
        g.fillEllipse(leftBallX - 6.0f, centerY - 6.0f, 12.0f, 12.0f);
        g.fillEllipse(rightBallX - 6.0f, centerY - 6.0f, 12.0f, 12.0f);
    }

    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);
}

VibeshifterEditor::VibeshifterEditor(VibeshifterProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), visualizer(p)
{
    addAndMakeVisible(visualizer);

    satModeBox.addItem("Light", 1);
    satModeBox.addItem("Medium", 2);
    satModeBox.addItem("Strong", 3);
    addAndMakeVisible(satModeBox);
    satModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processorRef.apvts, "SAT_MODE", satModeBox);

    wowSyncButton.setButtonText("Sync");
    addAndMakeVisible(wowSyncButton);
    wowSyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processorRef.apvts, "WOW_SYNC", wowSyncButton);

    auto setupRotaryKnob = [this](juce::Slider& slider, juce::Label& label, const juce::String& name)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 16);
        addAndMakeVisible(slider);

        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(11.0f, juce::Font::bold));
        addAndMakeVisible(label);
    };

    setupRotaryKnob(pitchSlider, pitchLabel, "Pitch");
    pitchAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "PITCH", pitchSlider);

    setupRotaryKnob(formantSlider, formantLabel, "Formant");
    formantAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "FORMANT", formantSlider);

    setupRotaryKnob(mixSlider, mixLabel, "Mix");
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "MIX", mixSlider);

    setupRotaryKnob(wowRateSlider, wowRateLabel, "Wow Rate");
    wowRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "WOW_RATE", wowRateSlider);

    setupRotaryKnob(wowDepthSlider, wowDepthLabel, "Wow Depth");
    wowDepthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "WOW_DEPTH", wowDepthSlider);

    setupRotaryKnob(satDriveSlider, satDriveLabel, "Sat Drive");
    satDriveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "SAT_DRIVE", satDriveSlider);

    setupRotaryKnob(hpfSlider, hpfLabel, "HPF");
    hpfAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "HPF", hpfSlider);

    setupRotaryKnob(lpfSlider, lpfLabel, "LPF");
    lpfAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "LPF", lpfSlider);

    setupRotaryKnob(spreadSlider, spreadLabel, "Spread");
    spreadAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "SPREAD", spreadSlider);

    setupRotaryKnob(chorusRateSlider, chorusRateLabel, "Chorus Rate");
    chorusRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "CHORUS_RATE", chorusRateSlider);

    setupRotaryKnob(chorusDepthSlider, chorusDepthLabel, "Chorus Depth");
    chorusDepthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "CHORUS_DEPTH", chorusDepthSlider);

    setupRotaryKnob(delayTimeSlider, delayTimeLabel, "Delay Time");
    delayTimeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "DELAY_TIME", delayTimeSlider);

    setupRotaryKnob(delayFbSlider, delayFbLabel, "Feedback");
    delayFbAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "DELAY_FEEDBACK", delayFbSlider);

    setupRotaryKnob(delayMixSlider, delayMixLabel, "Delay Mix");
    delayMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "DELAY_MIX", delayMixSlider);

    setSize(760, 560);
}

void VibeshifterEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff18181c));
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("MeatTheBeat.de", getLocalBounds().removeFromBottom(25), juce::Justification::centred, true);
}

void VibeshifterEditor::resized()
{
    auto area = getLocalBounds();
    visualizer.setBounds(area.removeFromTop(130).reduced(10));
    area.removeFromBottom(25);

    auto grid = area.reduced(10);
    int cols = 4;
    int rows = 4;
    int cellW = grid.getWidth() / cols;
    int cellH = grid.getHeight() / rows;

    auto placeControl = [](juce::Rectangle<int> bounds, juce::Component& comp, juce::Label* label = nullptr)
    {
        if (label)
        {
            label->setBounds(bounds.removeFromTop(16));
        }
        comp.setBounds(bounds.reduced(4));
    };

    auto r1 = grid.removeFromTop(cellH);
    placeControl(r1.removeFromLeft(cellW), pitchSlider, &pitchLabel);
    placeControl(r1.removeFromLeft(cellW), formantSlider, &formantLabel);
    placeControl(r1.removeFromLeft(cellW), mixSlider, &mixLabel);
    {
        auto sub = r1.removeFromLeft(cellW);
        wowSyncButton.setBounds(sub.removeFromTop(20).reduced(20, 0));
        placeControl(sub, wowRateSlider, &wowRateLabel);
    }

    auto r2 = grid.removeFromTop(cellH);
    placeControl(r2.removeFromLeft(cellW), wowDepthSlider, &wowDepthLabel);
    {
        auto sub = r2.removeFromLeft(cellW);
        satModeBox.setBounds(sub.removeFromTop(20).reduced(10, 0));
        placeControl(sub, satDriveSlider, &satDriveLabel);
    }
    placeControl(r2.removeFromLeft(cellW), hpfSlider, &hpfLabel);
    placeControl(r2.removeFromLeft(cellW), lpfSlider, &lpfLabel);

    auto r3 = grid.removeFromTop(cellH);
    placeControl(r3.removeFromLeft(cellW), spreadSlider, &spreadLabel);
    placeControl(r3.removeFromLeft(cellW), chorusRateSlider, &chorusRateLabel);
    placeControl(r3.removeFromLeft(cellW), chorusDepthSlider, &chorusDepthLabel);
    placeControl(r3.removeFromLeft(cellW), delayTimeSlider, &delayTimeLabel);

    auto r4 = grid.removeFromTop(cellH);
    placeControl(r4.removeFromLeft(cellW), delayFbSlider, &delayFbLabel);
    placeControl(r4.removeFromLeft(cellW), delayMixSlider, &delayMixLabel);
}
