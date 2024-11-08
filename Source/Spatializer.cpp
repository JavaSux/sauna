#include <string_view>
#include "Spatializer.h"

Spatializer::Spatializer(
    IPLContext context,
    int samplingRate,
    int frameSize
) :
    context{ context }
{
    // Init output buffer
    output = {};
    IPLAudioSettings audioSettings{ samplingRate, frameSize };
    steam_assert(
        iplAudioBufferAllocate(context, 2, frameSize, &output),
        "Failed to allocate output buffer"
    );

    // Init HRTF
    IPLHRTFSettings hrtfSettings{
        .type = IPL_HRTFTYPE_DEFAULT,
        .volume = 1.0f
    };

    hrtf = {};
    steam_assert(
        iplHRTFCreate(context, &audioSettings, &hrtfSettings, &hrtf),
        "Failed to create HRTF"
    );

    // Init effect
    IPLBinauralEffectSettings effectSettings{
        .hrtf = hrtf,
    };

    effect = {};
    steam_assert(
        iplBinauralEffectCreate(context, &audioSettings, &effectSettings, &effect),
        "Failed to create Binaural Effect"
    );

    // Init default effect params
    params = {
        .direction = IPLVector3{ 0.0f, 0.0f, -1.0f },
        .interpolation = IPL_HRTFINTERPOLATION_BILINEAR,
        .spatialBlend = 1.0f,
        .hrtf = hrtf,
        .peakDelays = nullptr
    };
}

Spatializer::~Spatializer() {
    // Free output buffer
    iplAudioBufferFree(context, &output);
    // Release effect
    iplBinauralEffectRelease(&effect);
    // Release HRTF
    iplHRTFRelease(&hrtf);
}

void Spatializer::setDirection(IPLVector3 direction) {
    params.direction = direction;
}

void Spatializer::apply(juce::AudioBuffer<float> &buffer, int input_channel_count) {
    if (buffer.getNumChannels() != 2)
        throw new std::runtime_error(std::format(
            "Effect requires exactly 2 channels, but was given {}",
            buffer.getNumChannels()
        ));

    IPLAudioBuffer input {
        .numChannels = std::min(input_channel_count, 2),
        .numSamples = buffer.getNumSamples(),
        // cast because Steam Audio is not const-correct
        .data = const_cast<float**>(buffer.getArrayOfReadPointers())
    };

    iplBinauralEffectApply(effect, &params, &input, &output);

    // Copy output back to buffer
    size_t buffer_size = input.numSamples * sizeof(float);
    std::memcpy(buffer.getWritePointer(0), output.data[0], buffer_size);
    std::memcpy(buffer.getWritePointer(1), output.data[1], buffer_size);
}