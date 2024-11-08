#include "SaunaProcessor.h"
#include "SaunaEditor.h"
#include <format>

SaunaProcessor::SaunaProcessor() :
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

SaunaProcessor::~SaunaProcessor() {
    spatializer.reset();
    iplContextRelease(&steam_audio_context);
}


juce::String const SaunaProcessor::getName() const {
    return JucePlugin_Name;
}

bool SaunaProcessor::acceptsMidi() const { return false; }
bool SaunaProcessor::producesMidi() const { return false; }
bool SaunaProcessor::isMidiEffect() const { return false; }
double SaunaProcessor::getTailLengthSeconds() const { return 0.0; }

int SaunaProcessor::getNumPrograms() {
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SaunaProcessor::getCurrentProgram() { return 0; }
void SaunaProcessor::setCurrentProgram(int index) {}
juce::String const SaunaProcessor::getProgramName(int index) { return {}; }
void SaunaProcessor::changeProgramName(int index, juce::String const &newName) {}


void SaunaProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    spatializer.emplace(steam_audio_context, (int) sampleRate, samplesPerBlock);
}

void SaunaProcessor::releaseResources() {
    spatializer.reset();
}

bool SaunaProcessor::isBusesLayoutSupported(BusesLayout const &layouts) const {
    // Output must be stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;

    // Input may be mono or stereo
    auto inputs = layouts.getMainInputChannelSet();
    if (inputs != juce::AudioChannelSet::stereo() && inputs != juce::AudioChannelSet::mono()) return false;
    
    return true;
}

void SaunaProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    Spatializer &effect = spatializer.value();

    auto position = playHead.load()->getPosition();
    float time = position.hasValue() ? position.operator*().getTimeInSeconds().orFallback(0.0) : 0.0;

    effect.setDirection(IPLVector3{
        (float) std::sin(time * 2.0) * 2.0f,
        (float) std::cos(time * 2.0) * 2.0f,
        0.0
    });

    try {
        auto time = std::chrono::system_clock::now();
        effect.apply(buffer, getMainBusNumInputChannels());
        debugMessage = std::format(
            "{}us",
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - time).count()
        );
    } catch (std::exception e) {
        debugMessage = e.what();
    }
}


bool SaunaProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* SaunaProcessor::createEditor() {
    return new SaunaEditor{ *this };
}


void SaunaProcessor::getStateInformation(juce::MemoryBlock &destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SaunaProcessor::setStateInformation(void const *data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout SaunaProcessor::createStateParameterLayout() {
    auto layout = juce::AudioProcessorValueTreeState::ParameterLayout{};

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ExampleParam", "Example param",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.1f),
        0.0
    ));

    return layout;
}


juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new SaunaProcessor();
}
