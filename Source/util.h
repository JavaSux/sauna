#pragma once

#include <array>
#include <format>
#include <JuceHeader.h>
#include <phonon.h>

// Coordinate where x is right, y is forward, and z is up
struct Vec3 {
    float x, y, z;

    Vec3() = default;
    constexpr Vec3(float x, float y, float z) : x{ x }, y{ y }, z{ z } {}
    constexpr Vec3(IPLVector3 const &steam) :
        x{ steam.x },
        y{ -steam.z },
        z{ steam.y }
    {}
    Vec3(std::array<juce::AudioParameterFloat *, 3> const &params) :
        x{ *params[0] },
        y{ *params[1] },
        z{ *params[2] }
    {}

    constexpr IPLVector3 toSteam() const {
        return IPLVector3 { x, z, -y };
    }

    constexpr bool hasDirection() const {
        return *this != Vec3{};
    }

    constexpr bool operator==(Vec3 const &other) const {
        return x == other.x
            && y == other.y
            && z == other.z;
    }
};


#pragma warning(disable: 4505) // MSVC allow unused
static void steam_assert(IPLerror status, std::string_view description) {
    std::string message;

    switch (status) {
    case IPL_STATUS_SUCCESS:
        return;
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
        message = std::format("{}: Unknown error code {}", description, (int) status);
        break;
    }

    throw new std::runtime_error(message);
}
