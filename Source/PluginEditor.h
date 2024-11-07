#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SaunaAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    SaunaAudioProcessorEditor(SaunaAudioProcessor&);
    ~SaunaAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SaunaAudioProcessor& audioProcessor;

    // Make non-copyable
    SaunaAudioProcessorEditor(const SaunaAudioProcessorEditor&) = delete;
    SaunaAudioProcessorEditor& operator=(SaunaAudioProcessorEditor const &) = delete;

    JUCE_LEAK_DETECTOR(SaunaAudioProcessorEditor)
};
