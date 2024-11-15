#include "SaunaControls.h"
#include "Spatializer.h"
#include "util.h"

#include <JuceHeader.h>
#include <format>
#include <numbers>

using std::numbers::pi;

static const std::array<char const *, 3> DIRECTION_NAMES{ "Right", "Forward", "Up" };

static std::array<juce::AudioParameterFloat *, 3> vectorParam(
	std::function<juce::AudioParameterFloat *(int, char)> &&constructor
) {
	return { constructor(0, 'X'), constructor(1, 'Y'), constructor(2, 'Z') };
}

SaunaControls::SaunaControls(juce::AudioProcessor &processor) :
	speed{ new juce::AudioParameterFloat("speed", "Speed", 0.01f, 10.0f, 1.0f) },
	phase{ new juce::AudioParameterFloat("phase", "Phase", 0.0f, float{ pi * 2.0 }, 0.0f) },
	mode{ new juce::AudioParameterChoice("mode", "Mode", { "Static", "Orbit", "Path" }, 0) },
	minDistance{ new juce::AudioParameterFloat("minDistance", "Min. Distance", 0.1f, 10.0f, 0.2f) },
	tempoSync{ new juce::AudioParameterBool("tempoSync", "Tempo sync", true) },

	staticPosition{ vectorParam([](int i, char axis) {
		return new juce::AudioParameterFloat(
			std::format("staticPosition{}", axis),
			DIRECTION_NAMES[i],
			-2.0f, 2.0f, DEFAULT_SOURCE_POSITION[i]
		);
	}) },
	orbitCenter{ vectorParam([](int, char axis) {
		return new juce::AudioParameterFloat(
			std::format("orbitCenter{}", axis),
			std::format("Orbit center {} position", axis),
			-10.0f, 10.0f, 0.0f
		);
	}) },
	orbitAxis{ vectorParam([](int i, char axis) {
		return new juce::AudioParameterFloat(
			std::format("orbitAxis{}", axis),
			std::format("Orbit axis {}", axis),
			-1.0f, 1.0f, DEFAULT_ORBIT_AXIS[i]
		);
	}) },

	orbitRadius  { new juce::AudioParameterFloat("orbitRadius",   "Orbit radius",  0.0f, 10.0f, 2.0f) },
	orbitStretch { new juce::AudioParameterFloat("orbitStretch",  "Orbit stretch", 0.0f, 10.0f, 1.0f) },
	orbitRotation{ new juce::AudioParameterFloat("orbitRotation", "Orbit stretch rotation", float{ -pi }, float{ pi }, 0.0f) }
{
	processor.addParameter(speed);
	processor.addParameter(phase);
	processor.addParameter(tempoSync);
	processor.addParameter(minDistance);
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
		return orbit(time);

	case SaunaMode::Path:
		return Vec3{};

	default:
		throw std::runtime_error{ std::format("Undefined mode {}", index) };
	}
}

float SaunaControls::getMinDistance() const {
	return *minDistance;
}

Vec3 SaunaControls::orbit(float time) const {
	float theta = (time + phase->get()) * speed->get();

	Vec3 point = Vec3::rotation2D(theta) * orbitRadius->get();
	point.x *= orbitStretch->get();
	point = point.rotateZ(orbitRotation->get());

	Vec3 axis{ Vec3{ orbitAxis }.normalized() };
	if (axis == Vec3::up()) {
		// no change
	} else if (axis == Vec3::down()) {
		point.x *= -1.0f;
	} else {
		point = point.axisAngleRotate(
			Vec3::up().cross(axis).normalized(), 
			std::acos(Vec3::up().dot(axis))
		);
	}

	return point + Vec3{ orbitCenter };
}