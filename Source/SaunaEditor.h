#pragma once

#include <JuceHeader.h>
#include "SaunaProcessor.h"
#include "Viewport.h"

const juce::Colour ACCENT_COLOR = juce::Colour::fromHSL(0.11f, 1.0f, 0.35f, 1.0);

struct ViewportFrameComponent: juce::Component {
    juce::Path roundRectPath;
    juce::DropShadow dropShadow;
    ViewportComponent viewport;

    ViewportFrameComponent(SaunaControls const &pluginState) :
        dropShadow{ juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.6f), 4, { 0, 2 } },
        viewport{ pluginState }
    {
        addAndMakeVisible(viewport);
        roundRectPath.addRoundedRectangle(0.0f, 0.0f, static_cast<float>(getWidth()), static_cast<float>(getHeight()) , 3.0f);
    }

	void resized() override {
        auto bounds = getLocalBounds().reduced(16);
		viewport.setBounds(bounds);

		roundRectPath.clear();
		roundRectPath.addRoundedRectangle(
            static_cast<float>(bounds.getX()     - 1), static_cast<float>(bounds.getY()      - 1),
            static_cast<float>(bounds.getWidth() + 2), static_cast<float>(bounds.getHeight() + 2),
            3.0f
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
	ControlPanelComponent controlPanel;

    JUCE_LEAK_DETECTOR(SaunaEditor)
};
