#pragma once

#include <JuceHeader.h>
#include "BinauralEffect.h"

class SaunaAudioProcessor : public juce::AudioProcessor {
public:
    SaunaAudioProcessor();
    ~SaunaAudioProcessor() override;


    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createStateParameterLayout();
    juce::AudioProcessorValueTreeState pluginState {
        *this,
        nullptr, // undo manager
        "Parameters",
        createStateParameterLayout(),
    };

    std::string debugMessage{};

private:
    IPLContext steam_audio_context{};
    std::optional<BinauralEffect> binaural{};

    // Make non-copyable
    SaunaAudioProcessor(const SaunaAudioProcessor&) = delete;
    SaunaAudioProcessor& operator= (const SaunaAudioProcessor&) = delete;

    JUCE_LEAK_DETECTOR(SaunaAudioProcessor)
};
