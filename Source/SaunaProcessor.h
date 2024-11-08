#pragma once

#include <JuceHeader.h>
#include "Spatializer.h"

struct SaunaProcessor : juce::AudioProcessor {
    SaunaProcessor();
    ~SaunaProcessor() override;


    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(BusesLayout const&layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    
    juce::String const getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    juce::String const getProgramName (int index) override;
    void changeProgramName (int index, const juce::String &newName) override;

    
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;
    
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
    std::optional<Spatializer> spatializer{};

    // Make non-copyable
    SaunaProcessor(SaunaProcessor const &) = delete;
    SaunaProcessor& operator= (SaunaProcessor const &) = delete;

    JUCE_LEAK_DETECTOR(SaunaProcessor)
};
