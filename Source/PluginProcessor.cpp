#include "PluginProcessor.h"
#include "PluginEditor.h"


SaunaAudioProcessor::SaunaAudioProcessor() :
    AudioProcessor{
        BusesProperties{}
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    }
{
    IPLContextSettings contextSettings{};
    contextSettings.version = STEAMAUDIO_VERSION;

    steam_assert(
        iplContextCreate(&contextSettings, &steam_audio_context),
        "Failed to initialize Steam Audio context"
    );
}

SaunaAudioProcessor::~SaunaAudioProcessor() {
    binaural.reset();
    iplContextRelease(&steam_audio_context);
}


const juce::String SaunaAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool SaunaAudioProcessor::acceptsMidi() const { return false; }
bool SaunaAudioProcessor::producesMidi() const { return false; }
bool SaunaAudioProcessor::isMidiEffect() const { return false; }
double SaunaAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int SaunaAudioProcessor::getNumPrograms() {
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SaunaAudioProcessor::getCurrentProgram() { return 0; }
void SaunaAudioProcessor::setCurrentProgram(int index) {}
const juce::String SaunaAudioProcessor::getProgramName(int index) { return {}; }
void SaunaAudioProcessor::changeProgramName(int index, const juce::String& newName) {}


void SaunaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    binaural.emplace(steam_audio_context, (int) sampleRate, samplesPerBlock);
}

void SaunaAudioProcessor::releaseResources() {
    binaural.reset();
}

bool SaunaAudioProcessor::isBusesLayoutSupported(const BusesLayout & layouts) const {
    // Check if the input layout matches the output layout
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet().size() > 0;
}

void SaunaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    BinauralEffect &effect = binaural.value();

    auto position = playHead.load()->getPosition();
    float time = position.hasValue() ? position.operator*().getTimeInSeconds().orFallback(0.0) : 0.0;

    effect.params.direction = IPLVector3{
        (float) std::sin(time * 2.0),
        (float) std::cos(time * 2.0),
        0.0
    };

    try {
        effect.apply(buffer, getMainBusNumInputChannels());
    } catch (std::exception e) {
        debugMessage = e.what();
    }
}


bool SaunaAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* SaunaAudioProcessor::createEditor() {
    return new SaunaAudioProcessorEditor{ *this };
}


void SaunaAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SaunaAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout SaunaAudioProcessor::createStateParameterLayout() {
    auto layout = juce::AudioProcessorValueTreeState::ParameterLayout{};

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ExampleParam", "Example param",
        juce::NormalisableRange<float>(0.0, 1.0, 0.1), 0.0
    ));

    return layout;
}


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SaunaAudioProcessor();
}
