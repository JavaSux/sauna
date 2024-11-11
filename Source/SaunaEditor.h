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
    std::optional<juce::OpenGLShaderProgram::Attribute> 
        position{},
        normal{},
        sourceColour{},
        textureCoordIn{};

    VertexAttributes() = default;
    VertexAttributes(juce::OpenGLShaderProgram &);

    void enable() const;
    void disable() const;
};


struct BufferHandle {
    bool owning;
    GLuint vertexBuffer, indexBuffer;
    GLsizei numIndices;

    BufferHandle() = delete;
    BufferHandle(BufferHandle const &) = delete;
    BufferHandle(BufferHandle &&other) noexcept :
        owning{ other.owning },
        vertexBuffer{ other.vertexBuffer },
        indexBuffer{ other.indexBuffer },
        numIndices{ other.numIndices }
    {
        other.owning = false;
    }

    BufferHandle(
        std::vector<Vertex> const &vertices, 
        std::vector<GLuint> const &indices
    ) :
        owning{ true },
        numIndices{ static_cast<GLsizei>(indices.size()) }
    {
        juce::gl::glGenBuffers(1, &vertexBuffer);
        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vertexBuffer);
        juce::gl::glBufferData(
            juce::gl::GL_ARRAY_BUFFER, 
            sizeof(Vertex) * vertices.size(), 
            vertices.data(), 
            juce::gl::GL_STATIC_DRAW
        );

        juce::gl::glGenBuffers(1, &indexBuffer);
        juce::gl::glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        juce::gl::glBufferData(
            juce::gl::GL_ELEMENT_ARRAY_BUFFER,
            sizeof(uint32_t) * numIndices,
            indices.data(),
            juce::gl::GL_STATIC_DRAW
        );
    }

    ~BufferHandle() {
        if (owning) {
            juce::gl::glDeleteBuffers(1, &vertexBuffer);
            juce::gl::glDeleteBuffers(1, &indexBuffer);
        }
    }
};

struct Mesh {
    std::vector<BufferHandle> bufferHandles{};

    Mesh() = default;
    Mesh(Mesh const &) = delete;
    Mesh(Mesh &&other) noexcept : bufferHandles{ std::move(other.bufferHandles) } {}
    Mesh &operator=(Mesh const &) = delete;
    Mesh &operator=(Mesh &&other) noexcept {
        bufferHandles = std::move(other.bufferHandles);
        return *this;
    };
    ~Mesh() = default;

    void pushQuad(float scale, std::array<float, 4> const &color);
};


struct ViewportRenderer: juce::OpenGLAppComponent {
    ViewportRenderer();
    ViewportRenderer(ViewportRenderer const &) = delete;
    ViewportRenderer &operator=(ViewportRenderer const &) = delete;
    ~ViewportRenderer();

    void initialise() override;
    void render() override;
    void paint(juce::Graphics &) override;
    void shutdown() override;

private:
    juce::OpenGLContext openGLContext;
    juce::OpenGLShaderProgram shaderProgram;
    std::optional<VertexAttributes> vertexAttributes;
    std::optional<juce::OpenGLShaderProgram::Uniform> projectionMatrixUniform;
    std::optional<juce::OpenGLShaderProgram::Uniform> viewMatrixUniform;
    Mesh mesh;
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
