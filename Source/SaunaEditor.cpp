#include "SaunaProcessor.h"
#include "SaunaEditor.h"

#include <string_view>
#include <stdint.h>
#include "util.h"

ControlPanelComponent::ControlPanelComponent(SaunaProcessor &p) : 
    saunaProcessor{ p },
    dropShadow{ juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.8f), 10, {} }
{
    leftPanel.setColour(juce::LookAndFeel_V4::ColourScheme::UIColour::widgetBackground, juce::Colours::red);
    rightPanel.setColour(juce::LookAndFeel_V4::ColourScheme::UIColour::widgetBackground, juce::Colours::blue);

    addAndMakeVisible(leftPanel);
    addAndMakeVisible(rightPanel);
}

void ControlPanelComponent::resized() {
    auto bounds = getLocalBounds();
    leftPanel.setBounds(bounds.removeFromLeft(200).reduced(8));
    rightPanel.setBounds(bounds.reduced(8));
}

void ControlPanelComponent::paint(juce::Graphics &graphics) {
	graphics.fillAll(juce::Colour::fromHSL(0.0f, 0.0f, 0.15f, 1.0));
    // Fake shadow of viewport frame
    dropShadow.drawForRectangle(graphics, { 0, -10, getWidth(), 10 });
}


SaunaEditor::SaunaEditor(SaunaProcessor &p) :
    AudioProcessorEditor{ &p },
    audioProcessor{ p },
    viewport{ viewportFrame.viewport },
	controlPanel{ p },
    constrainer{},
    resizer{ this, &constrainer }
{
    setSize(800, 600);
    setTitle("Sauna");

    addAndMakeVisible(viewportFrame);
    addAndMakeVisible(controlPanel);
    resizer.setAlwaysOnTop(true);
    addAndMakeVisible(resizer);
    constrainer.setMinimumSize(300, 250);
}

void SaunaEditor::paint(juce::Graphics &graphics) {
    graphics.fillAll(juce::Colour::fromHSL(0.0f, 0.0f, 0.28f, 1.0f));
}

void SaunaEditor::resized() {
	resizer.setBounds(getWidth() - 16, getHeight() - 16, 16, 16);

	auto bounds = getLocalBounds();
	controlPanel.setBounds(bounds.removeFromBottom(120));
	viewportFrame.setBounds(bounds);
}
