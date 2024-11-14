#pragma once

#include <JuceHeader.h>
#include "SaunaProcessor.h"

struct Vertex {
    std::array<float, 3> position;
    std::array<float, 3> normal;
    std::array<float, 4> colour;
    std::array<float, 2> texCoord;
};


struct VertexAttributes {
    using Attribute = juce::OpenGLShaderProgram::Attribute;

    // May be nullopt for a given shaderprogram if the attribute gets pruned
    std::optional<juce::OpenGLShaderProgram::Attribute> 
        position{},
        normal{},
        sourceColour{},
        textureCoordIn{};

    VertexAttributes() = delete;
    VertexAttributes(juce::OpenGLShaderProgram const &);
    VertexAttributes(VertexAttributes const &) = delete;
    VertexAttributes(VertexAttributes &&) noexcept = default;

    void enable() const;
    void disable() const;
};


struct Uniforms {
    using Uniform = juce::OpenGLShaderProgram::Uniform;

    // May be nullopt for a given shaderprogram if the uniform gets pruned
    std::optional<juce::OpenGLShaderProgram::Uniform>
        modelMatrix,
        viewMatrix,
        projectionMatrix;

    Uniforms() = delete;
    Uniforms(juce::OpenGLShaderProgram const &);
};

struct BufferHandle {
    bool owning;
    GLuint vertexBuffer, indexBuffer;
    GLsizei numIndices;

    BufferHandle() = delete;
    BufferHandle(BufferHandle &&other) noexcept;
    BufferHandle(BufferHandle const &) = delete;
    BufferHandle(
        std::vector<Vertex> const &vertices,
        std::vector<GLuint> const &indices
    );
    BufferHandle &operator=(BufferHandle &&) noexcept;
    BufferHandle &operator=(BufferHandle &) = delete;

    static BufferHandle quad(float scale, float z, juce::Colour const &color);

    ~BufferHandle() {
        if (owning) {
            juce::gl::glDeleteBuffers(1, &vertexBuffer);
            juce::gl::glDeleteBuffers(1, &indexBuffer);
        }
    }
};

struct Mesh {
    BufferHandle bufferHandle;
    std::shared_ptr<juce::OpenGLShaderProgram> shader;
    VertexAttributes attribs;
    Uniforms uniforms;
    juce::Matrix3D<float> modelMatrix;

    Mesh() = delete;
    Mesh(Mesh const &) = delete;
    Mesh &operator=(Mesh const &) = delete;

    Mesh(
        BufferHandle &&handle,
        std::shared_ptr<juce::OpenGLShaderProgram> &shader,
        juce::Matrix3D<float> modelMatrix = {}
    ) noexcept : 
        bufferHandle{ std::move(handle) },
        attribs{ *shader },
        uniforms{ *shader },
        shader{ shader },
        modelMatrix{ modelMatrix }
    {}
    Mesh(Mesh &&) noexcept = default;
    Mesh &operator=(Mesh &&) noexcept = default;
    ~Mesh() = default;
};


struct ViewportRenderer: juce::OpenGLAppComponent {
    static const juce::Point<float> INITIAL_MOUSE;
    static const juce::Colour CLEAR_COLOR;

    ViewportRenderer();
    ViewportRenderer(ViewportRenderer const &) = delete;
    ViewportRenderer &operator=(ViewportRenderer const &) = delete;
    ~ViewportRenderer();

    void update();

    void initialise() override;
    void render() override;
    void paint(juce::Graphics &) override;
    void shutdown() override;

    void mouseMove(juce::MouseEvent const &) override;
    void mouseExit(juce::MouseEvent const &) override;

private:
    juce::VBlankAttachment vBlankTimer;
    juce::Time lastUpdateTime;
    juce::Time startTime;

    juce::OpenGLContext openGLContext;

    std::shared_ptr<juce::OpenGLShaderProgram> gridFloorShader;
    std::shared_ptr<juce::OpenGLShaderProgram> ballShader;

    std::optional<Mesh> gridFloor;
    std::optional<Mesh> ball;

    // Relative window mouse position [-1, 1]
    juce::Point<float> mousePosition{ INITIAL_MOUSE };
    juce::Point<float> smoothMouse{ INITIAL_MOUSE };
};


struct SaunaEditor: juce::AudioProcessorEditor {
    SaunaEditor(SaunaProcessor&);
    SaunaEditor(const SaunaEditor&) = delete;
    SaunaEditor &operator=(SaunaEditor const &) = delete;
    ~SaunaEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SaunaProcessor &audioProcessor;
    ViewportRenderer viewport;

    JUCE_LEAK_DETECTOR(SaunaEditor)
};
