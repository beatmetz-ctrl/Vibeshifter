#include "PluginProcessor.h"
#include "PluginEditor.h"

StereoVocalShifterAudioProcessor::StereoVocalShifterAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout StereoVocalShifterAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("PITCH", 1), "Pitch (Semitones)",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FORMANT", 1), "Formant (Semitones)",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("MIX", 1), "Dry/Wet Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));

    return { params.begin(), params.end() };
}

void StereoVocalShifterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);
    for (int ch = 0; ch < 2; ++ch)
        shifters[ch].reset();

    setLatencySamples(PitchFormantShifter::fftSize);
}

void StereoVocalShifterAudioProcessor::releaseResources()
{
}

bool StereoVocalShifterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void StereoVocalShifterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    float pitchSemitones = apvts.getRawParameterValue("PITCH")->load();
    float formantSemitones = apvts.getRawParameterValue("FORMANT")->load();
    float mixPercent = apvts.getRawParameterValue("MIX")->load() / 100.0f;

    int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    int numSamples = buffer.getNumSamples();
    double sampleRate = getSampleRate();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);
        shifters[ch].processBlock(channelData, numSamples, sampleRate, pitchSemitones, formantSemitones, mixPercent);
    }
}

juce::AudioProcessorEditor* StereoVocalShifterAudioProcessor::createEditor()
{
    return new StereoVocalShifterAudioProcessorEditor(*this);
}

void StereoVocalShifterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void StereoVocalShifterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StereoVocalShifterAudioProcessor();
}
