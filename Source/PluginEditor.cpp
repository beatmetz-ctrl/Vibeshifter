#include "PluginEditor.h"

void ChromeCadillacLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&)
{
    const float radius = static_cast<float>(juce::jmin(width, height)) * 0.42f;
    const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Drop shadow
    g.setColour(juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.65f));
    g.fillEllipse(cx - radius - 3.0f, cy - radius + 2.0f, (radius + 3.0f) * 2.0f, (radius + 3.0f) * 2.0f);

    // Outer polished chrome bezel
    juce::ColourGradient chromeGrad(
        juce::Colour(0xfff5f7fa), cx - radius, cy - radius,
        juce::Colour(0xff4a5058), cx + radius, cy + radius, false);
    chromeGrad.addColour(0.35, juce::Colour(0xffc9d0d8));
    chromeGrad.addColour(0.65, juce::Colour(0xff78808a));
    g.setGradientFill(chromeGrad);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    // Speedometer dial face (dark 70s dashboard black)
    const float innerR = radius * 0.84f;
    g.setColour(juce::Colour(0xff101114));
    g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);

    // Tick marks around dial
    for (int i = 0; i <= 10; ++i)
    {
        const float t = static_cast<float>(i) / 10.0f;
        const float a = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
        const float r1 = innerR * 0.82f;
        const float r2 = innerR * 0.96f;
        const bool active = t <= sliderPos + 0.01f;
        g.setColour(active ? juce::Colour(0xffff5522) : juce::Colour(0xff555b66));
        g.drawLine(
            cx + std::sin(a) * r1, cy - std::cos(a) * r1,
            cx + std::sin(a) * r2, cy - std::cos(a) * r2,
            active ? 2.2f : 1.4f);
    }

    // Center brushed chrome cap
    const float capR = innerR * 0.56f;
    juce::ColourGradient capGrad(
        juce::Colour(0xffe8ecf0), cx - capR, cy - capR,
        juce::Colour(0xff5c626a), cx + capR, cy + capR, false);
    g.setGradientFill(capGrad);
    g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

    // Classic 70s glowing orange-red speedometer needle
    const float needleLen = innerR * 0.86f;
    g.setColour(juce::Colour(0xffff3b1f));
    juce::Path needle;
    needle.addRoundedRectangle(-2.0f, -needleLen, 4.0f, needleLen * 0.9f, 1.5f);
    g.fillPath(needle, juce::AffineTransform::rotation(angle).translated(cx, cy));
}

CadillacShifterAudioProcessorEditor::CadillacShifterAudioProcessorEditor(CadillacShifterAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(660, 340);

    setupKnob(pitchSlider, pitchLabel, "PITCH", " st");
    setupKnob(formantSlider, formantLabel, "FORMANT", " st");
    setupKnob(shifterMixSlider, shifterMixLabel, "SHIFTER MIX", " %");
    setupKnob(chorusMixSlider, chorusMixLabel, "JUNO CHORUS", " %");

    pitchAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "pitch", pitchSlider);
    formantAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "formant", formantSlider);
    shifterMixAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "shifterMix", shifterMixSlider);
    chorusMixAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "chorusMix", chorusMixSlider);
}

CadillacShifterAudioProcessorEditor::~CadillacShifterAudioProcessorEditor()
{
    pitchSlider.setLookAndFeel(nullptr);
    formantSlider.setLookAndFeel(nullptr);
    shifterMixSlider.setLookAndFeel(nullptr);
    chorusMixSlider.setLookAndFeel(nullptr);
}

void CadillacShifterAudioProcessorEditor::setupKnob(
    juce::Slider& slider, juce::Label& label, const juce::String& title, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 22);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff0e6d2));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff141418));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff6b7280));
    slider.setTextValueSuffix(suffix);
    slider.setLookAndFeel(&chromeLookAndFeel);
    addAndMakeVisible(slider);

    label.setText(title, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    label.setColour(juce::Label::textColourId, juce::Colour(0xffe5e7eb));
    addAndMakeVisible(label);
}

void CadillacShifterAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // 1. Outer 70s Polished Chrome Frame
    juce::ColourGradient outerChrome(
        juce::Colour(0xfff8fafc), 0.0f, 0.0f,
        juce::Colour(0xff525866), bounds.getWidth(), bounds.getHeight(), false);
    outerChrome.addColour(0.25, juce::Colour(0xffcbd5e1));
    outerChrome.addColour(0.55, juce::Colour(0xff64748b));
    outerChrome.addColour(0.85, juce::Colour(0xffe2e8f0));
    g.setGradientFill(outerChrome);
    g.fillRoundedRectangle(bounds, 14.0f);

    // 2. Inner Deep 70s Cadillac Candy-Burgundy / Espresso Metallic Dashboard Plate
    const auto plate = bounds.reduced(12.0f);
    juce::ColourGradient dashGrad(
        juce::Colour(0xff3b0d14), plate.getX(), plate.getY(),
        juce::Colour(0xff12080a), plate.getX(), plate.getBottom(), false);
    dashGrad.addColour(0.45, juce::Colour(0xff26090e));
    g.setGradientFill(dashGrad);
    g.fillRoundedRectangle(plate, 10.0f);

    // Chrome inner trim border
    g.setColour(juce::Colour(0xffd1d5db));
    g.drawRoundedRectangle(plate, 10.0f, 2.0f);

    // 3. Top 70s Chrome Radiator Grille Slats
    const float grilleY = plate.getY() + 10.0f;
    const float grilleH = 14.0f;
    g.setColour(juce::Colour(0xff0b0b0e));
    g.fillRoundedRectangle(plate.getX() + 18.0f, grilleY, plate.getWidth() - 36.0f, grilleH, 4.0f);
    for (float gx = plate.getX() + 26.0f; gx < plate.getRight() - 26.0f; gx += 10.0f)
    {
        g.setColour(juce::Colour(0xffcbd5e1));
        g.fillRect(gx, grilleY + 2.0f, 3.0f, grilleH - 4.0f);
    }

    // 4. Classic Cadillac V-Crest Emblem & Chrome Script Header
    const float cx = bounds.getCentreX();
    const float crestY = plate.getY() + 34.0f;

    juce::Path vCrest;
    vCrest.startNewSubPath(cx - 56.0f, crestY + 4.0f);
    vCrest.lineTo(cx, crestY + 20.0f);
    vCrest.lineTo(cx + 56.0f, crestY + 4.0f);
    vCrest.lineTo(cx + 50.0f, crestY + 9.0f);
    vCrest.lineTo(cx, crestY + 26.0f);
    vCrest.lineTo(cx - 50.0f, crestY + 9.0f);
    vCrest.closeSubPath();
    g.setColour(juce::Colour(0xffe5e7eb));
    g.fillPath(vCrest);

    g.setColour(juce::Colour(0xfff8fafc));
    g.setFont(juce::Font(juce::FontOptions(24.0f, juce::Font::bold | juce::Font::italic)));
    g.drawText("VibeShifter", 0, static_cast<int>(plate.getY() + 26.0f), getWidth(), 28, juce::Justification::centred);

    g.setColour(juce::Colour(0xfffbbf24));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText("MEATTHEBEAT  ·  SERIES '77  ·  FORMANT & PITCH  ·  JUNO CHORUS",
               0, static_cast<int>(plate.getY() + 56.0f), getWidth(), 16, juce::Justification::centred);

    // 5. Chrome corner rivets
    const float rOff = 22.0f;
    const float rSize = 7.0f;
    for (auto pt : { juce::Point<float>(rOff, rOff),
                     juce::Point<float>(bounds.getWidth() - rOff, rOff),
                     juce::Point<float>(rOff, bounds.getHeight() - rOff),
                     juce::Point<float>(bounds.getWidth() - rOff, bounds.getHeight() - rOff) })
    {
        g.setColour(juce::Colour(0xffe2e8f0));
        g.fillEllipse(pt.x - rSize * 0.5f, pt.y - rSize * 0.5f, rSize, rSize);
        g.setColour(juce::Colour(0xff334155));
        g.drawEllipse(pt.x - rSize * 0.5f, pt.y - rSize * 0.5f, rSize, rSize, 1.0f);
    }
}

void CadillacShifterAudioProcessorEditor::resized()
{
    const int topY = 108;
    const int knobW = 140;
    const int knobH = 160;
    const int gap = 16;
    const int totalW = 4 * knobW + 3 * gap;
    int startX = (getWidth() - totalW) / 2;

    auto layoutDial = [&](juce::Slider& s, juce::Label& l, int idx)
    {
        const int x = startX + idx * (knobW + gap);
        l.setBounds(x, topY, knobW, 20);
        s.setBounds(x, topY + 22, knobW, knobH);
    };

    layoutDial(pitchSlider, pitchLabel, 0);
    layoutDial(formantSlider, formantLabel, 1);
    layoutDial(shifterMixSlider, shifterMixLabel, 2);
    layoutDial(chorusMixSlider, chorusMixLabel, 3);
}
