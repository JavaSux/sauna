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

const int SAUNA_MODE_SIZE = 3;
enum struct SaunaMode: int {
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
	Vec3 getLastPosition() const { return lastPosition.load(); }

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

private:
	std::atomic<Vec3> lastPosition{}; // std::atomic falls back to Mutex for large types
	Vec3 orbit(float time) const;
};