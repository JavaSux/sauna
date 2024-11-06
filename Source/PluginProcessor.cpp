#include "PluginProcessor.h"
#include "PluginEditor.h"


SaunaAudioProcessor::SaunaAudioProcessor() : AudioProcessor(
    BusesProperties{}
    #ifndef JucePlugin_PreferredChannelConfigurations
        .withInput("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    #endif
) {}

SaunaAudioProcessor::~SaunaAudioProcessor() {}


const juce::String SaunaAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool SaunaAudioProcessor::acceptsMidi() const { return false; }
bool SaunaAudioProcessor::producesMidi() const { return false; }
bool SaunaAudioProcessor::isMidiEffect() const { return false; }

double SaunaAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int SaunaAudioProcessor::getNumPrograms() {
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SaunaAudioProcessor::getCurrentProgram() {
    return 0;
}

void SaunaAudioProcessor::setCurrentProgram(int index) {}

const juce::String SaunaAudioProcessor::getProgramName(int index) {
    return {};
}

void SaunaAudioProcessor::changeProgramName(int index, const juce::String& newName) {}


void SaunaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void SaunaAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SaunaAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Check if the input layout matches the output layout
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}
#endif

void SaunaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);

        // TODO: process the audio data
    }
}


bool SaunaAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SaunaAudioProcessor::createEditor() {
    return new SaunaAudioProcessorEditor (*this);
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
