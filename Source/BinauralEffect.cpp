#include <sstream>
#include <string_view>
#include "BinauralEffect.h"

void steam_assert(IPLerror status, std::string_view message) {
    if (status == IPL_STATUS_SUCCESS) { return; }

    std::ostringstream ss;
    ss << message.data() << ": ";
    switch (status) {
    case IPL_STATUS_FAILURE:
        ss << "Error";
        break;
    case IPL_STATUS_OUTOFMEMORY:
        ss << "Out of memory";
        break;
    case IPL_STATUS_INITIALIZATION:
        ss << "Initialization failure";
        break;
    default:
        ss << "Unknown error code '" << status << "'";
        break;
    }

    throw new std::runtime_error(ss.str());
}

BinauralEffect::BinauralEffect(
    IPLContext context,
    int samplingRate,
    int frameSize
) :
    context{ context }
{
    IPLAudioSettings audioSettings{ samplingRate, frameSize };
    steam_assert(
        iplAudioBufferAllocate(context, 2, frameSize, &output),
        "Failed to allocate output buffer"
    );

    IPLHRTFSettings hrtfSettings{};
    hrtfSettings.type = IPL_HRTFTYPE_DEFAULT;
    hrtfSettings.volume = 1.0f;

    steam_assert(
        iplHRTFCreate(context, &audioSettings, &hrtfSettings, &hrtf),
        "Failed to create HRTF"
    );

    IPLBinauralEffectSettings effectSettings{};
    effectSettings.hrtf = hrtf;

    steam_assert(
        iplBinauralEffectCreate(context, &audioSettings, &effectSettings, &effect),
        "Failed to create Binaural Effect"
    );

    params = {
        .direction = IPLVector3{ 1.0f, 1.0f, 1.0f },
        .interpolation = IPL_HRTFINTERPOLATION_NEAREST,
        .spatialBlend = 1.0f,
        .hrtf = hrtf,
        .peakDelays = nullptr
    };
}

BinauralEffect::~BinauralEffect() {
    if (output.numSamples != 0) {
        iplAudioBufferFree(context, &output);
    }
    iplBinauralEffectRelease(&effect);
    iplHRTFRelease(&hrtf);
}

IPLHRTF const& BinauralEffect::getHrtf() const { return hrtf; }

/* Must set `params` before calling */
void BinauralEffect::apply(juce::AudioBuffer<float>& buffer, int input_channel_count) {
    if (buffer.getNumChannels() == 0) {
        return;
    }

    for (int i = input_channel_count; i < buffer.getNumChannels(); i++) {
        buffer.clear(i, 0, buffer.getNumChannels());
    }

    IPLAudioBuffer input {
        .numChannels = std::min(input_channel_count, 2),
        .numSamples = buffer.getNumSamples(),
        .data = const_cast<float**>(buffer.getArrayOfReadPointers())
    };

    iplBinauralEffectApply(effect, &params, &input, &output);

    std::memcpy(buffer.getWritePointer(0), output.data[0], buffer.getNumSamples() * sizeof(float));
    std::memcpy(buffer.getWritePointer(1), output.data[1], buffer.getNumSamples() * sizeof(float));
}