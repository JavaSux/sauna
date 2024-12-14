#pragma once

#include <JuceHeader.h>
#include <phonon.h>
#include "util.h"

struct PathNode {
	juce::Vector3D<float> position;
	// Easing nextEase;
	// Interpolation nextPath;
	float nextDuration;
};

enum struct SaunaMode {
	Static,
	Orbit,
	Path,
};

struct SaunaControls {
	SaunaControls() = delete;
	SaunaControls(juce::AudioProcessor &);
	SaunaControls(SaunaControls const &) = delete;
	~SaunaControls() = default;

	Vec3 updatePosition(float time);
	Vec3 getLastPosition() const { return lastPosition; }
	float getMinDistance() const;

private:
	// Global params
	juce::AudioParameterChoice *mode;
	juce::AudioParameterFloat *speed;
	juce::AudioParameterFloat *phase;
	juce::AudioParameterBool *tempoSync;
	juce::AudioParameterFloat *minDistance;

	// Static params
	std::array<juce::AudioParameterFloat *, 3> staticPosition;

	// Orbit params
	std::array<juce::AudioParameterFloat *, 3> orbitCenter;
	std::array<juce::AudioParameterFloat *, 3> orbitAxis;

	juce::AudioParameterFloat *orbitRadius;
	juce::AudioParameterFloat *orbitStretch;
	juce::AudioParameterFloat *orbitRotation;

	// Path params
	std::vector<PathNode> nodes{};
	unsigned int currentNode{};

	Vec3 lastPosition{};
	Vec3 orbit(float time) const;
};