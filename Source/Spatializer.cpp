#include "Spatializer.h"

// Implementation for BinauralEffect
BinauralEffect::BinauralEffect(IPLContext context, IPLAudioSettings *audioSettings) :
    hrtf{}, effect{}
{
    IPLHRTFSettings hrtfSettings{
        .type = IPL_HRTFTYPE_DEFAULT, // built-in HRTF
        .volume = 1.0f, // 100% volume
        .normType = IPL_HRTFNORMTYPE_RMS // do normalize volume
    };

    steam_assert(
        iplHRTFCreate(context, audioSettings, &hrtfSettings, &hrtf),
        "Failed to create HRTF"
    );

    IPLBinauralEffectSettings binauralSettings{
        .hrtf = hrtf
    };

    steam_assert(
        iplBinauralEffectCreate(context, audioSettings, &binauralSettings, &effect),
        "Failed to create Binaural Effect"
    );

    params = {
        .direction = DEFAULT_SOURCE_POSITION.toSteam(),
        .interpolation = IPL_HRTFINTERPOLATION_BILINEAR, // HQ interpolation
        .spatialBlend = 1.0f, // 100% wet signal
        .hrtf = hrtf
    };
}

BinauralEffect::~BinauralEffect() {
    iplBinauralEffectRelease(&effect);
    iplHRTFRelease(&hrtf);
}

void BinauralEffect::setParams(Vec3 direction) {
    params.direction = (direction.isOrigin() ? DEFAULT_SOURCE_POSITION : direction).toSteam();
}

void BinauralEffect::processBlock(IPLAudioBuffer &input, IPLAudioBuffer &output) {
    iplBinauralEffectApply(effect, &params, &input, &output);
}


// Implementation for DirectEffect
DirectEffect::DirectEffect(IPLContext context, IPLAudioSettings *audioSettings) :
    effect{},
    attenuation{
        .type = IPL_DISTANCEATTENUATIONTYPE_INVERSEDISTANCE,
        .minDistance = 0.2f, // 100% volume at 20cm
    },
    airAbsorption{
        .type = IPL_AIRABSORPTIONTYPE_DEFAULT,
    },
    params{
        .flags = static_cast<IPLDirectEffectFlags>(IPL_DIRECTEFFECTFLAGS_APPLYDISTANCEATTENUATION | IPL_DIRECTEFFECTFLAGS_APPLYAIRABSORPTION),
    }
{
    IPLDirectEffectSettings directSettings{
        .numChannels = 2
    };
    steam_assert(
        iplDirectEffectCreate(context, audioSettings, &directSettings, &effect),
        "Failed to create Direct Effect"
    );

    setParams(context, DEFAULT_SOURCE_POSITION, 0.2f);
}

DirectEffect::~DirectEffect() {
    iplDirectEffectRelease(&effect);
}

// Set params and recalculate attenuation if they changed
void DirectEffect::setParams(IPLContext context, Vec3 position, float minDistance) {
    bool changed{ position != prevPosition };

    if (minDistance != attenuation.minDistance) {
        attenuation.minDistance = minDistance;
        changed = true;
    }

    if (changed) {
        prevPosition = position;
        IPLVector3 newPosition = position.toSteam();

        params.distanceAttenuation = iplDistanceAttenuationCalculate(
            context, 
            newPosition, 
            LISTENER_POSITION.toSteam(),
            &attenuation
        );
        iplAirAbsorptionCalculate(
            context,
            newPosition, 
            LISTENER_POSITION.toSteam(), 
            &airAbsorption, 
            params.airAbsorption
        );
    }
}

void DirectEffect::processBlock(IPLAudioBuffer buffer) {
    iplDirectEffectApply(effect, &params, &buffer, &buffer);
}


// Implementation for Spatializer
Spatializer::Spatializer(IPLContext context, IPLAudioSettings *audioSettings) :
    context{ context },
    binaural{ context, audioSettings },
    direct{ context, audioSettings },
    output{}
{
    steam_assert(
        iplAudioBufferAllocate(context, 2, audioSettings->frameSize, &output),
        "Failed to allocate output buffer"
    );
}

Spatializer::~Spatializer() {
    iplAudioBufferFree(context, &output);
}

Spatializer &Spatializer::setParams(Vec3 position, float minDistance) {
    binaural.setParams(position);
    direct.setParams(context, position, minDistance);
    return *this;
}

Spatializer &Spatializer::processBlock(juce::AudioBuffer<float> &buffer, int input_channel_count) {
    if (buffer.getNumChannels() != 2) {
        throw std::runtime_error{ std::format(
            "Effect requires exactly 2 channels, but was given {}",
            buffer.getNumChannels()
        ) };
    }

    IPLAudioBuffer input {
        .numChannels = std::min(input_channel_count, 2),
        .numSamples = buffer.getNumSamples(),
        // cast because Steam Audio is not const-correct
        .data = const_cast<float**>(buffer.getArrayOfReadPointers())
    };

    binaural.processBlock(input, output);
    direct.processBlock(output);

    // Copy output to buffer
    size_t buffer_size = input.numSamples * sizeof(float);
    std::memcpy(buffer.getWritePointer(0), output.data[0], buffer_size);
    std::memcpy(buffer.getWritePointer(1), output.data[1], buffer_size);

    return *this;
}