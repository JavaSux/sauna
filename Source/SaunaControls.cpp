#include "SaunaControls.h"
#include "Spatializer.h"
#include "util.h"

#include <format>
#include <numbers>

using std::numbers::pi;

static std::array<juce::AudioParameterFloat *, 3> vectorParam(
	std::function<juce::AudioParameterFloat *(int, char)> &&constructor
) {
	return { constructor(0, 'X'), constructor(1, 'Y'), constructor(2, 'Z') };
}

SaunaControls::SaunaControls(juce::AudioProcessor &processor) :
	speed{ new juce::AudioParameterFloat("speed", "Speed", 0.01f, 10.0f, 1.0f) },
	tempoSync{ new juce::AudioParameterBool("tempoSync", "Tempo sync", true) },
	mode{ new juce::AudioParameterChoice("mode", "Mode", { "Static", "Orbit", "Path" }, 0) },
	minDistance{ new juce::AudioParameterFloat("minDistance", "Min. Distance", 0.1f, 10.0f, 0.2f) },
	staticPosition{
		new juce::AudioParameterFloat(
			"staticPositionX", "Right",
			-2.0f, 2.0f, DEFAULT_SOURCE_POSITION.x
		),
		new juce::AudioParameterFloat(
			"staticPositionY", "Forward",
			-2.0f, 2.0f, DEFAULT_SOURCE_POSITION.y
		),
		new juce::AudioParameterFloat(
			"staticPositionZ", "Up",
			-2.0f, 2.0f, DEFAULT_SOURCE_POSITION.z
		),
	},
	orbitCenter{ vectorParam([](int, char axis) {
		return new juce::AudioParameterFloat(
			std::format("orbitCenter{}", axis),
			std::format("Orbit center {} position", axis),
			-10.0f, 10.0f, 0.0f
		);
	}) },
	orbitAxis{ vectorParam([](int, char axis) {
		return new juce::AudioParameterFloat(
			std::format("orbitAxis{}", axis),
			std::format("Orbit axis {} component", axis),
			-1.0f, 1.0f, 0.0f
		);
	}) },
	orbitRadius  { new juce::AudioParameterFloat("orbitRadius",   "Orbit radius",  0.0f, 10.0f, 2.0f) },
	orbitStretch { new juce::AudioParameterFloat("orbitStretch",  "Orbit stretch", 0.0f, 10.0f, 1.0f) },
	orbitRotation{ new juce::AudioParameterFloat("orbitRotation", "Orbit stretch rotation", float{ -pi }, float{ pi }, 0.0f) }
{
	processor.addParameter(speed);
	processor.addParameter(tempoSync);
	processor.addParameter(mode);
	for (auto * ptr : staticPosition) processor.addParameter(ptr);
	for (auto * ptr : orbitCenter   ) processor.addParameter(ptr);
	for (auto * ptr : orbitAxis     ) processor.addParameter(ptr);
	processor.addParameter(orbitRadius);
	processor.addParameter(orbitStretch);
	processor.addParameter(orbitRotation);
}

Vec3 SaunaControls::getPositionFor(float time) const {
	auto index = mode->getIndex();

	switch (static_cast<SaunaMode>(index)) {
	case SaunaMode::Static:
		return Vec3{ staticPosition };

	case SaunaMode::Orbit:
		return Vec3{};

	case SaunaMode::Path:
		return Vec3{};

	default:
		throw new std::runtime_error(std::format("Undefined mode {}", index));
	}
}

float SaunaControls::getMinDistance() const {
	return *minDistance;
}