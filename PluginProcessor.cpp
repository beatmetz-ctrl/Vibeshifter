#include "PluginProcessor.h"
#include "PluginEditor.h"

VibeshifterProcessor::VibeshifterProcessor()
#imarker:base
    : AudioProcessor(BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

VibeshifterProcessor::~VibeshifterProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VibeshifterProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("PITCH", "Pitch", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("FORMANT", "Formant", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MIX", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("WOW_RATE", "Wow Rate", juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("WOW_DEPTH", "Wow Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("WOW_SYNC", "Wow Sync", false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>("SAT_MODE", "Sat Mode", juce::StringArray{"Light", "Medium", "Strong"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SAT_DRIVE", "Sat Drive", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("HPF", "HPF", juce::NormalisableRange<float>(20.0f, 1000.0f, 1.0f, 0.5f), 20.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LPF", "LPF", juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.5f), 20000.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SPREAD", "Spread", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("CHORUS_RATE", "Chorus Rate", juce::NormalisableRange<float>(0.1f, 5.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("CHORUS_DEPTH", "Chorus Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("DELAY_TIME", "Delay Time", juce::NormalisableRange<float>(1.0f, 1000.0f, 1.0f), 250.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DELAY_FEEDBACK", "Delay Feedback", juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DELAY_MIX", "Delay Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    return { params.begin(), params.end() };
}

const juce::String VibeshifterProcessor::getName() const { return JucePlugin_Name; }
bool VibeshifterProcessor::acceptsMidi() const { return false; }
bool VibeshifterProcessor::producesMidi() const { return false; }
bool VibeshifterProcessor::isMidiEffect() const { return false; }
double VibeshifterProcessor::getTailLengthSeconds() const { return 0.0; }

int VibeshifterProcessor::getNumPrograms() { return 1; }
int VibeshifterProcessor::getCurrentProgram() { return 0; }
void VibeshifterProcessor::setCurrentProgram(int) {}
const juce::String VibeshifterProcessor::getProgramName(int) { return {}; }
void VibeshifterProcessor::changeProgramName(int, const juce::String&) {}

void VibeshifterProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
}

void VibeshifterProcessor::releaseResources() {}

bool VibeshifterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void VibeshifterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // RMS Level für Visualizer ermitteln
    float rms = 0.0f;
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        rms += buffer.getRMSLevel(channel, 0, buffer.getNumSamples());
    }
    rms = (totalNumInputChannels > 0) ? (rms / totalNumInputChannels) : 0.0f;

    visualizerLevel.store(rms);
    visualizerSpread.store(apvts.getRawParameterValue("SPREAD")->load());
    visualizerChorusRate.store(apvts.getRawParameterValue("CHORUS_RATE")->load());
    visualizerDelayActive.store(apvts.getRawParameterValue("DELAY_MIX")->load() > 0.01f);

    // DSP-Verarbeitung erfolgt hier
}

bool VibeshifterProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* VibeshifterProcessor::createEditor()
{
    return new VibeshifterEditor(*this);
}

void VibeshifterProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VibeshifterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibeshifterProcessor();
}
