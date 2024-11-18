#pragma once

#include <JuceHeader.h>
#include "SaunaProcessor.h"
#include "Viewport.h"

const juce::Colour ACCENT_COLOR = juce::Colour::fromHSL(0.11f, 1.0f, 0.35f, 1.0);

struct ViewportFrameComponent: juce::Component {
    juce::Path roundRectPath;
    juce::DropShadow dropShadow;
    ViewportComponent viewport;

    ViewportFrameComponent() :
        dropShadow{ juce::Colour::fromFloatRGBA(0.0, 0.0, 0.0, 0.6), 4, { 0, 2 } }
    {
        addAndMakeVisible(viewport);
        roundRectPath.addRoundedRectangle(0, 0, getHeight(), getHeight() , 3);
    }

	void resized() override {
        auto bounds = getLocalBounds().reduced(16);
		viewport.setBounds(bounds);

		roundRectPath.clear();
		roundRectPath.addRoundedRectangle(
            bounds.getX() - 1, bounds.getY() - 1, 
            bounds.getWidth() + 2, bounds.getHeight() + 2, 
            3
        );
	}

    // Can't draw border over OpenGL because OpenGL is forced to render on top of everything else
    void paint(juce::Graphics &graphics) override {
        dropShadow.drawForPath(graphics, roundRectPath);
		graphics.setColour(ACCENT_COLOR);
		graphics.strokePath(roundRectPath, juce::PathStrokeType(2.0f));
    }
};

struct ControlPanelComponent: juce::Component {
    SaunaProcessor &saunaProcessor;
    juce::Component leftPanel, rightPanel;
    juce::DropShadow dropShadow;

    ControlPanelComponent() = delete;
    ControlPanelComponent(SaunaProcessor &p);
    ControlPanelComponent(ControlPanelComponent const &) = delete;
    ControlPanelComponent &operator=(ControlPanelComponent const &) = delete;
    ~ControlPanelComponent() override = default;

    void paint(juce::Graphics &) override;
    void resized() override;
};

struct SaunaEditor: juce::AudioProcessorEditor {
    SaunaEditor(SaunaProcessor&);
    SaunaEditor(const SaunaEditor&) = delete;
    SaunaEditor &operator=(SaunaEditor const &) = delete;
    ~SaunaEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SaunaProcessor &audioProcessor;
	juce::ComponentBoundsConstrainer constrainer;
    juce::ResizableCornerComponent resizer;
	ViewportFrameComponent viewportFrame;
    ViewportComponent &viewport;
	ControlPanelComponent controlPanel;

    JUCE_LEAK_DETECTOR(SaunaEditor)
};
