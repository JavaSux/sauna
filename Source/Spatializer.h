#pragma once

#include <JuceHeader.h>
#include <phonon.h>
#include <format>

static void steam_assert(IPLerror status, std::string_view description) {
    if (status == IPL_STATUS_SUCCESS) { return; }

    std::string message;
    switch (status) {
    case IPL_STATUS_FAILURE:
        message = std::format("{}: Internal error", description);
        break;
    case IPL_STATUS_OUTOFMEMORY:
        message = std::format("{}: Out of memory", description);
        break;
    case IPL_STATUS_INITIALIZATION:
        message = std::format("{}: Initialization failure", description);
        break;
    default:
        message = ""; // std::format("{}: Unknown error code '{}'", description, status);
        break;
    }

    throw new std::runtime_error(message);
}

struct Spatializer {
    Spatializer(
        IPLContext context, 
        int samplingRate, 
        int frameSize
    );
    ~Spatializer();
    Spatializer(Spatializer &) = delete;
    Spatializer& operator=(Spatializer const &) = delete;

    IPLHRTF const &getHrtf() const { return hrtf; }
    void setDirection(IPLVector3 direction);
    void setInterpQuality(bool hq);
    void apply(juce::AudioBuffer<float> &buffer, int input_channels);

private:
    IPLContext context;
    IPLAudioBuffer output;
    IPLBinauralEffectParams params;
    
    IPLBinauralEffect effect;
    IPLHRTF hrtf;
};