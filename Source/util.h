#pragma once

#include <array>
#include <span>
#include <format>
#include <JuceHeader.h>
#include <phonon.h>
#include <stdexcept>

// Coordinate where x is right, y is forward, and z is up
struct Vec3 {
    float x, y, z;

    constexpr Vec3() : x{ 0.0f }, y{ 0.0f }, z{ 0.0f } {}
    constexpr Vec3(float value) : x{ value }, y{ value }, z{ value } {}
    constexpr Vec3(float x, float y, float z) : x{ x }, y{ y }, z{ z } {}
    constexpr Vec3(IPLVector3 const &steam) :
        x{  steam.x },
        y{ -steam.z },
        z{  steam.y }
    {}
    Vec3(std::span<juce::AudioParameterFloat const * const, 3> params) :
        x{ *params[0] },
        y{ *params[1] },
        z{ *params[2] }
    {}

    constexpr static Vec3 origin() { return Vec3{}; }
    constexpr static Vec3 forward() { return Vec3{ 0.0f, 1.0f, 0.0f }; }
    constexpr static Vec3 up() { return Vec3{ 0.0f, 0.0f, 1.0f }; }
    constexpr static Vec3 down() { return Vec3{ 0.0f, 0.0f, -1.0f }; }

    constexpr static Vec3 rotation2D(float theta) {
        return Vec3{ std::cos(theta), std::sin(theta), 0.0f };
    }

    float magnitude() const {
        return std::sqrt((x * x) + (y * y) + (z * z));
    }

    constexpr Vec3 normalized() const {
        return *this / magnitude();
    }

    constexpr IPLVector3 toSteam() const {
        return IPLVector3 { x, z, -y };
    }

    juce::Vector3D<float> toJuce() const {
        return juce::Vector3D<float>{ x, y, z };
    }

    constexpr bool hasDirection() const {
        return *this != Vec3{};
    }

    constexpr Vec3 axisAngleRotate(Vec3 const &axis, float angle) const {
        return *this 
            + (axis.cross(*this) * std::sin(angle))
            + (axis.cross(axis.cross(*this)) * (1.0f - std::cos(angle)));
    }

    Vec3 rotateZ(float angle) const {
        float
            cos = std::cos(angle),
            sin = std::sin(angle);

        return Vec3{
            x * cos - y * sin,
            x * sin + y * cos,
            z
        };
    }

    constexpr Vec3 cross(Vec3 const &other) const {
        return Vec3{
            y * other.z - z * other.y,
            -(x * other.z - z * other.x),
            (x * other.y - y * other.x)
        };
    }

    constexpr float dot(Vec3 const &other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    constexpr bool operator==(Vec3 const &other) const {
        return x == other.x
            && y == other.y
            && z == other.z;
    }

    constexpr Vec3 operator*(float value) const {
        return Vec3{ x * value, y * value, z * value };
    }
    
    constexpr void operator*=(float value) {
        *this = *this * value;
    }

    constexpr Vec3 operator/(float value) const {
        return Vec3{ x / value, y / value, z / value };
    }

    constexpr Vec3 operator+(Vec3 const &other) const {
        return Vec3{ x + other.x, y + other.y, z + other.z };
    }

    constexpr void operator+=(Vec3 const &other) {
        *this = *this + other;
    }

    constexpr float operator[](int i) const {
        switch (i) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        default: throw std::out_of_range{ std::format("No element {} on Vec3", i) };
        }
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

    throw std::runtime_error{ message };
}

#define OPENGL_ASSERT() \
    if (juce::juce_isRunningUnderDebugger()) { \
        GLenum err = juce::gl::glGetError(); \
        if (err != juce::gl::GL_NO_ERROR) { \
            DBG("OpenGL error at " << __FILE__ << ":" << __LINE__ << ", code " << (int) err); \
            jassertfalse; \
        } \
    }



template<typename Data>
static Data expEase(Data const &current, Data const &target, float easing, float delta) {
    return target + (current - target) * std::exp(-easing * delta);
}

static std::shared_ptr<juce::OpenGLShaderProgram> loadShader(
    juce::OpenGLContext &context,
    juce::String &vertexSource,
    juce::String &fragmentSource,
    char const * shaderName
) {
    auto shader = std::make_shared<juce::OpenGLShaderProgram>(context);

    if (!shader->addVertexShader(vertexSource)) {
        DBG("\n\nVertex Shader Error in " << shaderName << ":\n" << shader->getLastError().toStdString());
        jassertfalse;
    }
    if (!shader->addFragmentShader(fragmentSource)) {
        DBG("\n\nFragment Shader Error in " << shaderName << ":\n" << shader->getLastError().toStdString());
        jassertfalse;
    }
    if (!shader->link()) {
        DBG("\n\nShader Link Error in " << shaderName << ":\n" << shader->getLastError().toStdString());
        jassertfalse;
    }

    return shader;
}