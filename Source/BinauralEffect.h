#pragma once

#include <JuceHeader.h>
#include <phonon.h>

void steam_assert(IPLerror status, std::string_view message);

struct BinauralEffect {
    IPLBinauralEffectParams params;

	BinauralEffect(
		IPLContext context, 
		int samplingRate, 
		int frameSize
	);
	~BinauralEffect();

	void apply(juce::AudioBuffer<float>& buffer, int input_channels);
	IPLHRTF const& getHrtf() const;

private:
	BinauralEffect(BinauralEffect&) = delete;
	BinauralEffect& operator=(BinauralEffect const&) = delete;

	IPLContext context;
	
	IPLAudioBuffer output;
	IPLBinauralEffect effect{};
	IPLHRTF hrtf{};
};