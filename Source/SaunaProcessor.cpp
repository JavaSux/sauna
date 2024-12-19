#include "SaunaProcessor.h"
#include "SaunaEditor.h"
#include <format>

static juce::AudioProcessorValueTreeState::ParameterLayout buildLayout() {
    auto layout = juce::AudioProcessorValueTreeState::ParameterLayout{};

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "StaticPosition", "Example param",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.1f),
        0.0f
    ));

    return layout;
}

SaunaProcessor::SaunaProcessor() :
    AudioProcessor{
        BusesProperties{}
            .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    },
    controls{ *this }
{
    IPLContextSettings contextSettings{
        .version = STEAMAUDIO_VERSION,
    };

    steam_assert(
        iplContextCreate(&contextSettings, &steam_audio_context),
        "Failed to initialize Steam Audio context"
    );

    juce::Logger::outputDebugString("Test");
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
void SaunaProcessor::setCurrentProgram(int) {}
juce::String const SaunaProcessor::getProgramName(int) { return {}; }
void SaunaProcessor::changeProgramName(int, juce::String const &) {}


void SaunaProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    IPLAudioSettings audioSettings{
        .samplingRate = static_cast<int>(sampleRate),
        .frameSize = samplesPerBlock,
    };
    spatializer.emplace(steam_audio_context, &audioSettings);
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

void SaunaProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &) {
    juce::ScopedNoDenormals noDenormals;

    try {
        auto playheadPosition = playHead.load()->getPosition();
        double time = playheadPosition.hasValue() ? playheadPosition->getTimeInSeconds().orFallback(0.0) : 0.0;

        auto position = controls.updatePosition(static_cast<float>(time));

        spatializer.value()
            .setParams(position, controls.minDistance->get())
            .processBlock(buffer, getMainBusNumInputChannels());
    } catch (std::exception e) {
        DBG(e.what());
    }
}


bool SaunaProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* SaunaProcessor::createEditor() {
    return new SaunaEditor{ *this };
}


void SaunaProcessor::getStateInformation(juce::MemoryBlock &) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SaunaProcessor::setStateInformation(void const *, int) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new SaunaProcessor();
}
