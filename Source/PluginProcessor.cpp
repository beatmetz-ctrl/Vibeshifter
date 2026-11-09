#include "PluginProcessor.h"
#include "PluginEditor.h"

CadillacShifterAudioProcessor::CadillacShifterAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout CadillacShifterAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "pitch", 1 },
        "Pitch",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "formant", 1 },
        "Formant",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "shifterMix", 1 },
        "Shifter Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "chorusMix", 1 },
        "Juno Chorus",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        0.0f));

    return { params.begin(), params.end() };
}

void CadillacShifterAudioProcessor::prepareToPlay(double sampleRate, int)
{
    shifterL.prepare(sampleRate);
    shifterR.prepare(sampleRate);
    junoChorus.prepare(sampleRate);
    setLatencySamples(PitchFormantShifter::kLatencySamples / 2);
}

void CadillacShifterAudioProcessor::releaseResources()
{
}

bool CadillacShifterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void CadillacShifterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    const float pitchSt = apvts.getRawParameterValue("pitch")->load();
    const float formantSt = apvts.getRawParameterValue("formant")->load();
    const float shifterMix = apvts.getRawParameterValue("shifterMix")->load() / 100.0f;
    const float chorusMix = apvts.getRawParameterValue("chorusMix")->load() / 100.0f;

    if (numChannels >= 1)
        shifterL.process(buffer.getWritePointer(0), numSamples, pitchSt, formantSt, shifterMix);

    if (numChannels >= 2)
        shifterR.process(buffer.getWritePointer(1), numSamples, pitchSt, formantSt, shifterMix);

    if (numChannels >= 2)
        junoChorus.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples, chorusMix);
    else if (numChannels == 1)
        junoChorus.processStereo(buffer.getWritePointer(0), nullptr, numSamples, chorusMix);
}

juce::AudioProcessorEditor* CadillacShifterAudioProcessor::createEditor()
{
    return new CadillacShifterAudioProcessorEditor(*this);
}

void CadillacShifterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CadillacShifterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CadillacShifterAudioProcessor();
}
