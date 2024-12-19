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


struct CPStaticControls: juce::Component {
    SaunaControls& controls;

    juce::Slider positionSliders[3];

    CPStaticControls(SaunaControls& controls);
    CPStaticControls(CPStaticControls const&) = delete;
    CPStaticControls& operator=(CPStaticControls const&) = delete;
    ~CPStaticControls() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
};


struct CPOrbitControls: juce::Component {
	SaunaControls& controls;

	juce::Slider centerSliders[3], axisSliders[3];
	juce::Slider radiusSlider, stretchSlider, rotationSlider;

	CPOrbitControls(SaunaControls& controls);
	CPOrbitControls(CPOrbitControls const&) = delete;
	CPOrbitControls& operator=(CPOrbitControls const&) = delete;
	~CPOrbitControls() override = default;

	void paint(juce::Graphics&) override;
	void resized() override;
};


struct CPPathControls: juce::Component {
	SaunaControls& controls;

	CPPathControls(SaunaControls& controls);
	CPPathControls(CPPathControls const&) = delete;
	CPPathControls& operator=(CPPathControls const&) = delete;
	~CPPathControls() override = default;

	void paint(juce::Graphics&) override;
	void resized() override;
};


struct CPModeControls: juce::Component {
    SaunaControls &controls;

    CPOrbitControls orbitControls;
    CPStaticControls staticControls;
    CPPathControls pathControls;

    CPModeControls(SaunaControls& controls);
    CPModeControls(CPModeControls const&) = delete;
    CPModeControls &operator=(CPModeControls const&) = delete;
    ~CPModeControls() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
};


struct CPCommonControls: juce::Component {
	SaunaControls &controls;
	CPModeControls &modeControls;

    juce::ComboBox modeComboBox;
    juce::Slider speedSlider, phaseSlider;
    juce::ToggleButton tempoSyncButton;

    CPCommonControls() = delete;
	CPCommonControls(SaunaControls& controls, CPModeControls &modeControls);
	CPCommonControls(CPCommonControls const&) = delete;
	CPCommonControls& operator=(CPCommonControls const&) = delete;
	~CPCommonControls() override = default;

	void paint(juce::Graphics&) override;
	void resized() override;
};


struct ControlPanelComponent: juce::Component {
    SaunaControls &controls;
    juce::DropShadow dropShadow;

	CPCommonControls commonControls;
    CPModeControls modeControls;

    ControlPanelComponent() = delete;
    ControlPanelComponent(SaunaControls &controls);
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
