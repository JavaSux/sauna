#pragma once

#include <JuceHeader.h>
#include <phonon.h>
#include "util.h"

const Vec3 DEFAULT_SOURCE_POSITION{ 0.0f, 0.5f, 0.0f }; // Straight ahead
const Vec3 DEFAULT_ORBIT_AXIS{ Vec3::up() };
const Vec3 LISTENER_POSITION{ Vec3::origin() };

struct BinauralEffect {
    BinauralEffect(IPLContext context, IPLAudioSettings *audioSettings);
    BinauralEffect(BinauralEffect const &) = delete;
    BinauralEffect &operator=(BinauralEffect const &) = delete;
    ~BinauralEffect();

    IPLHRTF const &getHrtf() const { return hrtf; }

    void setParams(Vec3 direction);
    void processBlock(IPLAudioBuffer &input, IPLAudioBuffer &output);

private:
    IPLHRTF hrtf;
    IPLBinauralEffect effect;
    IPLBinauralEffectParams params;
};

struct DirectEffect {
    DirectEffect(IPLContext context, IPLAudioSettings *audioSettings);
    DirectEffect(DirectEffect const &) = delete;
    DirectEffect &operator=(DirectEffect const &) = delete;
    ~DirectEffect();

    void setParams(IPLContext context, Vec3 position, float minDistance);
    void processBlock(IPLAudioBuffer buffer);

private:
    IPLDirectEffect effect;
    IPLDirectEffectParams params;
    IPLDistanceAttenuationModel attenuation;
    IPLAirAbsorptionModel airAbsorption;

    Vec3 prevPosition;
};

struct Spatializer {
    Spatializer(IPLContext context, IPLAudioSettings *audioSettings);
    Spatializer(Spatializer &) = delete;
    Spatializer & operator=(Spatializer const &) = delete;
    ~Spatializer();

    IPLHRTF const &getHrtf() const { return binaural.getHrtf(); }
    Spatializer &setParams(Vec3 position, float minDistance);
    Spatializer &processBlock(juce::AudioBuffer<float> &buffer, int input_channels);

private:
    IPLContext context;
    IPLAudioBuffer output;
    
    BinauralEffect binaural;
    DirectEffect direct;
};