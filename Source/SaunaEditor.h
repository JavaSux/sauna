#pragma once

#include <JuceHeader.h>
#include "SaunaProcessor.h"

struct SaunaEditor : juce::AudioProcessorEditor {
    SaunaEditor(SaunaProcessor&);
    ~SaunaEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SaunaProcessor& audioProcessor;

    // Make non-copyable
    SaunaEditor(const SaunaEditor&) = delete;
    SaunaEditor& operator=(SaunaEditor const &) = delete;

    JUCE_LEAK_DETECTOR(SaunaEditor)
};
