#include "SaunaProcessor.h" 
#include "SaunaEditor.h"

#include <string_view>
#include <stdint.h>
#include "util.h"

void sliderCopyParameter(juce::Slider &slider, juce::AudioParameterFloat *parameter, std::string name) {
	if (parameter != nullptr) {
		parameter->setValueNotifyingHost(static_cast<float>(slider.getValue()));
	} else {
		DBG("Tried to set value of nullptr audio parameter float for "<< name);
		JUCE_BREAK_IN_DEBUGGER
	}

	slider.setRange(parameter->range.start, parameter->range.end, parameter->range.interval);
	slider.setValue(parameter->get());
	slider.onValueChange = [parameter, &slider, name] {
		if (parameter != nullptr) {
			DBG("Update on " << name << " to " << slider.getValue());
			float sliderValue = static_cast<float>(slider.getValue());
			parameter->setValueNotifyingHost(sliderValue);
		} else {
			DBG("Tried to set value of nullptr audio parameter float for "<< name);
			JUCE_BREAK_IN_DEBUGGER
		}
	};
}


CPStaticControls::CPStaticControls(SaunaControls& controls) : controls{ controls } {
	for (int i = 0; i < 3; i++) {
		addAndMakeVisible(positionSliders[i]);
		sliderCopyParameter(positionSliders[i], controls.staticPosition[i], std::format("Static position[{}]", i));
		positionSliders[i].setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft, false, 64, 20);
	}
}

void CPStaticControls::paint(juce::Graphics& graphics) {
	graphics.fillAll(juce::Colours::magenta); // Debug magenta
}

void CPStaticControls::resized() {
	auto bounds = getLocalBounds();
	for (auto &slider : positionSliders) {
		slider.setBounds(bounds.removeFromTop(24));
	}
}


CPOrbitControls::CPOrbitControls(SaunaControls& controls) : controls{ controls } {
	for (int i = 0; i < 3; i++) {
		sliderCopyParameter(centerSliders[i], controls.orbitCenter[i], std::format("Orbit center[{}]", i));
		centerSliders[i].setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
		centerSliders[i].setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 64, 20);
		addAndMakeVisible(centerSliders[i]);

		sliderCopyParameter(axisSliders[i], controls.orbitAxis[i], std::format("Orbit axis[{}]", i));
		axisSliders[i].setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
		axisSliders[i].setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 64, 20);
		addAndMakeVisible(axisSliders[i]);
	}

	sliderCopyParameter(radiusSlider, controls.orbitRadius, "Orbit radius");
	radiusSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
	radiusSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft, false, 64, 20);
	addAndMakeVisible(radiusSlider);

	sliderCopyParameter(stretchSlider, controls.orbitStretch, "Orbit stretch");
	stretchSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
	stretchSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 64, 20);
	addAndMakeVisible(stretchSlider);

	sliderCopyParameter(rotationSlider, controls.orbitRotation, "Orbit rotation");
	rotationSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
	rotationSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 64, 20);
	addAndMakeVisible(rotationSlider);
}

void CPOrbitControls::paint(juce::Graphics& graphics) {
	graphics.fillAll(juce::Colours::blue); // Debug blue
}

void CPOrbitControls::resized() {
	auto bounds = getLocalBounds();
	for (int i = 0; i < 3; i++) {
		centerSliders[i].setBounds(bounds.removeFromTop(24));
		axisSliders[i].setBounds(bounds.removeFromTop(24));
	}
	radiusSlider.setBounds(bounds.removeFromTop(24));
	stretchSlider.setBounds(bounds.removeFromTop(24));
	rotationSlider.setBounds(bounds.removeFromTop(24));
}


CPPathControls::CPPathControls(SaunaControls& controls) : controls{ controls } {}

void CPPathControls::paint(juce::Graphics& graphics) {
	graphics.fillAll(juce::Colours::red); // Debug red
}

void CPPathControls::resized() {}


CPCommonControls::CPCommonControls(SaunaControls& controls, CPModeControls& modeControls) : 
	controls{ controls },
	modeControls{ modeControls } 
{
	addAndMakeVisible(modeComboBox);
	for (int i = 0; i < SAUNA_MODE_SIZE; i++) {
		modeComboBox.addItem(controls.mode->getAllValueStrings()[i], i + 1);
	}
	modeComboBox.setSelectedItemIndex(0);

	modeComboBox.onChange = [this] {
		float value = static_cast<float>(modeComboBox.getSelectedItemIndex()) / static_cast<float>(SAUNA_MODE_SIZE);
		this->controls.mode->setValueNotifyingHost(value);

		this->modeControls.staticControls.setVisible(false);
		this->modeControls.orbitControls .setVisible(false);
		this->modeControls.pathControls  .setVisible(false);

		switch (static_cast<SaunaMode>(modeComboBox.getSelectedItemIndex())) {
		case SaunaMode::Static:
			this->modeControls.staticControls.setVisible(true);
			break;
		case SaunaMode::Orbit:
			this->modeControls.orbitControls.setVisible(true);
			break;
		case SaunaMode::Path:
			this->modeControls.pathControls.setVisible(true);
			break;
		default:
			DBG("Undefined mode" << modeComboBox.getSelectedId());
			JUCE_BREAK_IN_DEBUGGER
		}
	};

	sliderCopyParameter(speedSlider, controls.speed, "Common speed");
	speedSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
	speedSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 64, 20);
	addAndMakeVisible(speedSlider);

	sliderCopyParameter(phaseSlider, controls.phase, "Common phase");
	phaseSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
	phaseSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 64, 20);
	addAndMakeVisible(phaseSlider);

	tempoSyncButton.setButtonText("Tempo Sync");
	tempoSyncButton.setToggleState(controls.tempoSync->get(), juce::NotificationType::dontSendNotification);
	addAndMakeVisible(tempoSyncButton);
}

void CPCommonControls::paint(juce::Graphics& graphics) {
	graphics.fillAll(juce::Colours::green);
}

void CPCommonControls::resized() {
	auto bounds = getLocalBounds();
	modeComboBox.setBounds(bounds.removeFromTop(24));
	speedSlider.setBounds(bounds.removeFromTop(24));
	phaseSlider.setBounds(bounds.removeFromTop(24));
	tempoSyncButton.setBounds(bounds.removeFromTop(24));
}

CPModeControls::CPModeControls(SaunaControls& controls) :
	controls{ controls },
	staticControls{ controls },
	orbitControls{ controls },
	pathControls{ controls }
{
	addAndMakeVisible(staticControls);
	addChildComponent(orbitControls);
	addChildComponent(pathControls);
}

void CPModeControls::paint(juce::Graphics& graphics) {
	graphics.fillAll(juce::Colours::yellow); // Debug yellow
}

void CPModeControls::resized() {
	auto bounds = getLocalBounds();
	staticControls.setBounds(bounds);
	orbitControls.setBounds(bounds);
	pathControls.setBounds(bounds);
}

ControlPanelComponent::ControlPanelComponent(SaunaControls &controls) : 
    controls{ controls },
    dropShadow{ juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.8f), 10, {} },
	commonControls{ controls, modeControls },
	modeControls{ controls }
{
	addAndMakeVisible(commonControls);
	addAndMakeVisible(modeControls);
}

void ControlPanelComponent::resized() {
    auto bounds = getLocalBounds();
    commonControls.setBounds(bounds.removeFromLeft(200).reduced(8));
    modeControls.setBounds(bounds.reduced(8));
}

void ControlPanelComponent::paint(juce::Graphics &graphics) {
	graphics.fillAll(juce::Colour::fromHSL(0.0f, 0.0f, 0.15f, 1.0));
    // Fake shadow of viewport frame
	dropShadow.drawForRectangle(graphics, { 0, -10, getWidth(), 10 });
}


SaunaEditor::SaunaEditor(SaunaProcessor &processor) :
    AudioProcessorEditor{ &processor },
    audioProcessor{ processor },
	controlPanel{ processor.getControls() },
    constrainer{},
    resizer{ this, &constrainer },
	viewportFrame{ processor.getControls() }
{
    setSize(640, 480);
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
