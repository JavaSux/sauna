#pragma once

#include <JuceHeader.h>
#include <array>
#include <optional>

#include "util.h"

constexpr int RASTER_SUPERSAMPLE = 2;
constexpr int BLOOM_PASSES = 7; // Downsampling means that higher pass counts are cheap
constexpr int BLOOM_DOWNSAMPLE = 2;
constexpr float BLOOM_STRENGTH = 0.125f;

struct GLVertex {
    std::array<float, 3> position;
    std::array<float, 3> normal;
    std::array<float, 4> colour;
    std::array<float, 2> texCoord;
};


struct GLVertexAttributes {
    using Attribute = juce::OpenGLShaderProgram::Attribute;

    // May be nullopt for a given shaderprogram if the attribute gets pruned
    std::optional<juce::OpenGLShaderProgram::Attribute> 
        position{},
        normal{},
        color{},
        texCoord{};

    GLVertexAttributes() = delete;
    GLVertexAttributes(GLVertexAttributes const &) = delete;
    GLVertexAttributes(GLVertexAttributes &&) noexcept = default;

    GLVertexAttributes(juce::OpenGLShaderProgram const &shader) {
        auto tryEmplace{ [&](std::optional<Attribute> &location, GLchar const *name) {
            if (juce::gl::glGetAttribLocation(shader.getProgramID(), name) >= 0) {
                location.emplace(shader, name);
            } else {
                location.reset();
            }
        } };

        tryEmplace(position, "aPosition");
        tryEmplace(normal, "aNormal");
        tryEmplace(color, "aColor");
        tryEmplace(texCoord, "aTexCoord");
    }

    void enable() const {
        size_t offset{ 0 };

        auto vertexBuffer{ [&](std::optional<juce::OpenGLShaderProgram::Attribute> const &attrib, GLint size) {
            if (attrib) {
                juce::gl::glVertexAttribPointer(
                    attrib->attributeID, size,
                    juce::gl::GL_FLOAT, juce::gl::GL_FALSE,
                    sizeof(GLVertex), reinterpret_cast<GLvoid *>(sizeof(float) * offset)
                );
                juce::gl::glEnableVertexAttribArray(attrib->attributeID);
            }

            offset += size;
        } };

        vertexBuffer(position, 3);
        vertexBuffer(normal, 3);
        vertexBuffer(color, 4);
        vertexBuffer(texCoord, 2);
    }

    void disable() const {
        auto disable{ [](std::optional<Attribute> const &attrib) {
            if (attrib) {
                juce::gl::glDisableVertexAttribArray(attrib->attributeID);
            }
        } };

        disable(position);
        disable(normal);
        disable(color);
        disable(texCoord);
    }
};


struct GLMeshUniforms {
    using Uniform = juce::OpenGLShaderProgram::Uniform;

    // May be nullopt for a given shaderprogram if the uniform gets pruned
    juce::OpenGLShaderProgram::Uniform
        modelMatrix,
        viewMatrix,
        projectionMatrix;

    GLMeshUniforms() = delete;
    GLMeshUniforms(GLMeshUniforms const &) = delete;
    GLMeshUniforms(GLMeshUniforms &&) noexcept = default;
    ~GLMeshUniforms() = default;

    GLMeshUniforms(juce::OpenGLShaderProgram const &shader) :
        modelMatrix{ shader, "modelMatrix" },
        viewMatrix{ shader, "viewMatrix" },
        projectionMatrix{ shader, "projectionMatrix" }
    {}
};


struct GLBufferHandle {
    bool owning;
    GLuint vertexBuffer, indexBuffer;
    GLsizei numIndices;

    GLBufferHandle() = delete;
    GLBufferHandle(GLBufferHandle const &) = delete;
    GLBufferHandle &operator=(GLBufferHandle &) = delete;

    GLBufferHandle(GLBufferHandle &&other) noexcept {
        this->~GLBufferHandle();

        owning = other.owning;
        vertexBuffer = other.vertexBuffer;
        indexBuffer = other.indexBuffer;
        numIndices = other.numIndices;

        other.owning = false;
    }

    GLBufferHandle(std::span<const GLVertex> vertices, std::span<const GLuint> indices) :
        owning{ true },
        numIndices{ static_cast<GLsizei>(indices.size()) }
    {
        juce::gl::glGenBuffers(1, &vertexBuffer);
        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vertexBuffer);
        juce::gl::glBufferData(
            juce::gl::GL_ARRAY_BUFFER, 
            vertices.size_bytes(), 
            vertices.data(), 
            juce::gl::GL_STATIC_DRAW
        );

        juce::gl::glGenBuffers(1, &indexBuffer);
        juce::gl::glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        juce::gl::glBufferData(
            juce::gl::GL_ELEMENT_ARRAY_BUFFER,
            indices.size_bytes(),
            indices.data(),
            juce::gl::GL_STATIC_DRAW
        );
        OPENGL_ASSERT();
    }

    ~GLBufferHandle() {
        if (owning) {
            juce::gl::glDeleteBuffers(1, &vertexBuffer);
            juce::gl::glDeleteBuffers(1, &indexBuffer);
        }
    }

    GLBufferHandle &operator=(GLBufferHandle &&other) noexcept {
        owning = other.owning;
        vertexBuffer = other.vertexBuffer;
        indexBuffer = other.indexBuffer;
        numIndices = other.numIndices;

        other.owning = false;

        return *this;
    }

    static GLBufferHandle quad(float size, juce::Colour const &color) {
        float scale = size / 2.0f;

        std::array<float, 4> colorRaw{
            color.getFloatRed(),
            color.getFloatGreen(),
            color.getFloatBlue(),
            color.getFloatAlpha()
        };
        std::vector<GLVertex> vertices{
            GLVertex{
                .position = { -scale, -scale, 0.0f },
                .colour = colorRaw,
                .texCoord = { 0.0, 0.0 }
        },
            GLVertex{
                .position = { scale, -scale, 0.0f },
                .colour = colorRaw,
                .texCoord = { 1.0, 0.0 }
        },
            GLVertex{
                .position = { -scale, scale, 0.0f },
                .colour = colorRaw,
                .texCoord = { 0.0, 1.0 }
        },
            GLVertex{
                .position = { scale, scale, 0.0f },
                .colour = colorRaw,
                .texCoord = { 1.0, 1.0 }
        }
        };
        std::vector<GLuint> indices{
            0, 1, 2,
            2, 1, 3
        };

        return { vertices, indices };
	}

	void drawElements() const {
        juce::gl::glDrawElements(juce::gl::GL_TRIANGLES, numIndices, juce::gl::GL_UNSIGNED_INT, nullptr);
    }
};


struct GLMesh {
    GLBufferHandle bufferHandle;
    std::shared_ptr<juce::OpenGLShaderProgram> shader;
    GLVertexAttributes attribs;
    GLMeshUniforms uniforms;
    juce::Matrix3D<float> modelMatrix;

    GLMesh() = delete;
    GLMesh(GLMesh const &) = delete;
    GLMesh &operator=(GLMesh const &) = delete;

    GLMesh(
        GLBufferHandle &&handle,
        std::shared_ptr<juce::OpenGLShaderProgram> &shader,
        juce::Matrix3D<float> modelMatrix = {}
    ) noexcept : 
        bufferHandle{ std::move(handle) },
        attribs{ *shader },
        uniforms{ *shader },
        shader{ shader },
        modelMatrix{ modelMatrix }
    {}
    GLMesh(GLMesh &&) noexcept = default;
    GLMesh &operator=(GLMesh &&) noexcept = default;
    ~GLMesh() = default;
};


struct GLBackBuffer {
    bool owning{ true };
    GLuint frameBuffer{ 0 }, outputTexture{ 0 }, depthStencilBuffer{ 0 };
    juce::Point<int> resolution;

    GLBackBuffer(GLBackBuffer const &) = delete;
    GLBackBuffer &operator=(GLBackBuffer const &) = delete;
    GLBackBuffer(GLBackBuffer &&other) noexcept {
        this->~GLBackBuffer();

        owning = other.owning;
        frameBuffer = other.frameBuffer;
        outputTexture = other.outputTexture;
        depthStencilBuffer = other.depthStencilBuffer;
        resolution = other.resolution;

        other.owning = false;
    };
    GLBackBuffer &operator=(GLBackBuffer &&other) noexcept {
        this->~GLBackBuffer();

        owning = other.owning;
        frameBuffer = other.frameBuffer;
        outputTexture = other.outputTexture;
        depthStencilBuffer = other.depthStencilBuffer;
        resolution = other.resolution;

        other.owning = false;
        return *this;
    }

    GLBackBuffer(juce::Point<int> resolution, bool useDepthStencil) : resolution{ resolution } {
        using namespace juce::gl;

        glGenFramebuffers(1, &frameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

        // Use texture for color info
        glGenTextures(1, &outputTexture);
        glBindTexture(GL_TEXTURE_2D, outputTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, resolution.x, resolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
        
        if (useDepthStencil) {
            glGenRenderbuffers(1, &depthStencilBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, depthStencilBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, resolution.x, resolution.y);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilBuffer);
        } else {
			depthStencilBuffer = 0;
        }

        jassert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        OPENGL_ASSERT();
    }

	void bindTexture(GLuint textureSlot) const {
		using namespace juce::gl;
		glActiveTexture(GL_TEXTURE0 + textureSlot);
		glBindTexture(GL_TEXTURE_2D, outputTexture);
	}

    void setRenderTarget(bool clear, juce::Point<int> bounds) const {
        using namespace juce::gl;

        glBindFramebuffer(juce::gl::GL_FRAMEBUFFER, frameBuffer);
        if (clear) {
            glViewport(0, 0, resolution.x, resolution.y);
            glClearColor(0.0, 0.0, 0.0, 0.0);
            glClear(GL_COLOR_BUFFER_BIT);
        }
		glViewport(0, 0, bounds.x, bounds.y);
    }

    void setRenderTarget(bool clear = false) const {
        setRenderTarget(clear, resolution);
    }

	void blitInto(GLuint outputBuffer) const {
		using namespace juce::gl;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, outputBuffer);
		glBlitFramebuffer(
			0, 0, resolution.x, resolution.y,
			0, 0, resolution.x, resolution.y,
			GL_COLOR_BUFFER_BIT, GL_NEAREST
		);
	}

    ~GLBackBuffer() {
        if (owning) {
            if (depthStencilBuffer) {
                juce::gl::glDeleteRenderbuffers(1, &depthStencilBuffer);
            }
            juce::gl::glDeleteTextures(1, &outputTexture);
            juce::gl::glDeleteFramebuffers(1, &frameBuffer);
        }
    }
};


struct PostProcess {
    using Uniform = juce::OpenGLShaderProgram::Uniform;

    GLBufferHandle fullscreenQuad;

    GLBackBuffer 
        rasterBuffer, 
        compositingBuffer, 
        bufferA, 
        bufferB;

    GLVertexAttributes 
        downsampleAttribs, 
        cinematicAttribs, 
        gaussianAttribs, 
        bloomAccumulateAttribs;

	std::shared_ptr<juce::OpenGLShaderProgram> 
        downsampleShader, 
        cinematicShader, 
        gaussianShader, 
        bloomAccumulateShader;

    Uniform 
        supersampleUniform, 
        renderedImageUniform, 
        downsampledImageUniform, 
        gaussianSourceTextureUniform,
        gaussianSourceResolutionUniform,
        gaussianVerticalUniform,
        bloomStrengthUniform,
        bloomDownsampleRatioUniform,
        bloomSourceTextureUniform;

    PostProcess(PostProcess const &) = delete;
    PostProcess(PostProcess &&) noexcept = default;
    PostProcess &operator=(PostProcess const &) = delete;
    PostProcess &operator=(PostProcess &&) noexcept = default;

    PostProcess(
        std::shared_ptr<juce::OpenGLShaderProgram> &downsampleShader,
        std::shared_ptr<juce::OpenGLShaderProgram> &cinematicShader,
        std::shared_ptr<juce::OpenGLShaderProgram> &gaussianShader,
        std::shared_ptr<juce::OpenGLShaderProgram> &bloomAccumulateShader,
        juce::Point<int> viewportSize,
        int supersample
    ) noexcept :
        fullscreenQuad{ GLBufferHandle::quad(2.0, juce::Colours::black) },
        downsampleAttribs{ *downsampleShader },
        cinematicAttribs{ *cinematicShader },
		gaussianAttribs{ *gaussianShader },
		bloomAccumulateAttribs{ *bloomAccumulateShader },

        supersampleUniform  { *downsampleShader, "supersample" },
        renderedImageUniform{ *downsampleShader, "renderedImage" },

        downsampledImageUniform{ *cinematicShader, "downsampledImage" },

		gaussianSourceTextureUniform{ *gaussianShader, "sourceTexture" },
		gaussianSourceResolutionUniform{ *gaussianShader, "sourceResolution" },
		gaussianVerticalUniform     { *gaussianShader, "vertical" },

		bloomStrengthUniform       { *bloomAccumulateShader, "strength" },
		bloomDownsampleRatioUniform{ *bloomAccumulateShader, "downsampleRatio" },
		bloomSourceTextureUniform  { *bloomAccumulateShader, "sourceTexture" },

        downsampleShader{ downsampleShader },
        cinematicShader{ cinematicShader },
		gaussianShader{ gaussianShader },
		bloomAccumulateShader{ bloomAccumulateShader },

        rasterBuffer{ viewportSize * supersample, true },
        compositingBuffer{ viewportSize, false },
		bufferA{ viewportSize, false },
		bufferB{ viewportSize, false }
    {}

    ~PostProcess() = default;

    void sizeTo(juce::Point<int> viewportSize, int supersample) {
        using namespace juce::gl;

        if (rasterBuffer.resolution != viewportSize * supersample) {
            rasterBuffer = GLBackBuffer{ viewportSize * supersample, true };
            compositingBuffer = GLBackBuffer{ viewportSize, false };
            bufferA = GLBackBuffer{ viewportSize, false };
            bufferB = GLBackBuffer{ viewportSize, false };
        }
    }

    void process(GLuint outputBuffer, bool skipVFX) const {
        using namespace juce::gl;

        glBindBuffer(GL_ARRAY_BUFFER, fullscreenQuad.vertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fullscreenQuad.indexBuffer);

		// Downsample into compositingBuffer
        compositingBuffer.setRenderTarget();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        downsampleShader->use();
        if (supersampleUniform.uniformID >= 0) { supersampleUniform.set(RASTER_SUPERSAMPLE); }
        if (renderedImageUniform.uniformID >= 0) { renderedImageUniform.set(0); } // GL_TEXTURE0

        downsampleAttribs.enable();
        rasterBuffer.bindTexture(0);
        fullscreenQuad.drawElements();
        downsampleAttribs.disable();

        if (skipVFX) {
			compositingBuffer.blitInto(outputBuffer);
			return;
        }

        // Bloom
        if (compositingBuffer.resolution == bufferA.resolution) {
			compositingBuffer.blitInto(bufferA.frameBuffer);
        } else if (compositingBuffer.resolution / 2 == bufferA.resolution) {
            // Downsample 2x
            bufferA.setRenderTarget(true);
            downsampleShader->use();
            if (supersampleUniform.uniformID >= 0) { supersampleUniform.set(2); }
            if (renderedImageUniform.uniformID >= 0) { renderedImageUniform.set(0); } // GL_TEXTURE0

            downsampleAttribs.enable();
            compositingBuffer.bindTexture(0);
            fullscreenQuad.drawElements();
            downsampleAttribs.disable();
        } else {
            jassertfalse; // Unhandled bloom downsampling ratio
        }

        GLBackBuffer const *vertical{ &bufferA }, *horizontal{ &bufferB };
        juce::Point<int> bloomDownsampleBounds{ vertical->resolution };

        for (int pass{ 0 }; ; ++pass) {
            // 1. Gaussian blur X from vertical to horizontal
            gaussianShader->use();
            if (gaussianSourceTextureUniform.uniformID >= 0) { gaussianSourceTextureUniform.set(0); } // GL_TEXTURE0
			if (gaussianSourceResolutionUniform.uniformID >= 0) { gaussianSourceResolutionUniform.set(bloomDownsampleBounds.toFloat().x, bloomDownsampleBounds.toFloat().y); }
            if (gaussianVerticalUniform.uniformID >= 0) { gaussianVerticalUniform.set(0); } // Horizontally

            gaussianAttribs.enable();

            horizontal->setRenderTarget(true, bloomDownsampleBounds);
            vertical->bindTexture(0);
            fullscreenQuad.drawElements();

            // 2. Gaussian blur Y from horizontal to vertical
            if (gaussianVerticalUniform.uniformID >= 0) { gaussianVerticalUniform.set(1); } // Vertically
            vertical->setRenderTarget(false, bloomDownsampleBounds); // Can skip clearing because downscale cleared it
			horizontal->bindTexture(0);
            fullscreenQuad.drawElements();

            gaussianAttribs.disable();

            // 3. Additive blend vertical to compositing buffer
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);

            // Downsample relative to vertical resolution because UV coordinates scale with texture bounds
            juce::Point<float> downsample = vertical->resolution.toFloat() / bloomDownsampleBounds.toFloat();
            compositingBuffer.setRenderTarget();
            bloomAccumulateShader->use();
            if (bloomStrengthUniform.uniformID >= 0) { bloomStrengthUniform.set(BLOOM_STRENGTH); }
            if (bloomDownsampleRatioUniform.uniformID >= 0) { bloomDownsampleRatioUniform.set(downsample.x, downsample.y); }
            if (bloomSourceTextureUniform.uniformID >= 0) { bloomSourceTextureUniform.set(0); } // GL_TEXTURE0

            bloomAccumulateAttribs.enable();
            vertical->bindTexture(0);
            fullscreenQuad.drawElements();
            bloomAccumulateAttribs.disable();

            glDisable(GL_BLEND);

            // Skip unnecessary final-pass work
			if (pass == BLOOM_PASSES - 1) { break; }

            // 4. Downsample vertical to horizontal
            bloomDownsampleBounds /= BLOOM_DOWNSAMPLE;
            horizontal->setRenderTarget(true, bloomDownsampleBounds);
			downsampleShader->use();
			if (supersampleUniform.uniformID >= 0) { supersampleUniform.set(BLOOM_DOWNSAMPLE); }
			if (renderedImageUniform.uniformID >= 0) { renderedImageUniform.set(0); } // GL_TEXTURE0

			downsampleAttribs.enable();
			vertical->bindTexture(0);
			fullscreenQuad.drawElements();
			downsampleAttribs.disable();
            
            // 5. Swap buffers
			std::swap(horizontal, vertical);
		}


        // Cinematic
        glBindFramebuffer(GL_FRAMEBUFFER, outputBuffer);
        glViewport(0, 0, compositingBuffer.resolution.x, compositingBuffer.resolution.y);
        cinematicShader->use();
        if (downsampledImageUniform.uniformID >= 0) { downsampledImageUniform.set(0); } // GL_TEXTURE0

        cinematicAttribs.enable();
        compositingBuffer.bindTexture(0);
        fullscreenQuad.drawElements();
        cinematicAttribs.disable();
    }
};


struct ViewportComponent: juce::OpenGLAppComponent {
    static const juce::Point<float> INITIAL_MOUSE;
    static const juce::Colour CLEAR_COLOR;
    static const double MOUSE_DELAY;

    ViewportComponent(ViewportComponent const &) = delete;
    ViewportComponent &operator=(ViewportComponent const &) = delete;

    ViewportComponent() :
        juce::OpenGLAppComponent{},
        gridFloorShader{ nullptr },
        startTime{ juce::Time::getCurrentTime() },
		lastUpdateTime{ startTime },
        vBlankTimer{ this, [this](){ update(); } }
    {};

    ~ViewportComponent() {
        shutdownOpenGL();
    }

    void update();

    void initialise() override;
    void recomputeViewportSize();
    void render() override;
    void resized() override;
    void paint(juce::Graphics &) override {}; // Cannot paint over OpenGL Components
    void shutdown() override;

    void mouseMove(juce::MouseEvent const &) override;
    void mouseEnter(juce::MouseEvent const &) override;
    void mouseExit(juce::MouseEvent const &) override;

private:
    juce::VBlankAttachment vBlankTimer;
    juce::Time startTime;
    juce::Time lastUpdateTime;

    juce::OpenGLContext openGLContext;
    juce::Rectangle<int> componentBounds, renderBounds;
    std::optional<PostProcess> postprocess;

    std::shared_ptr<juce::OpenGLShaderProgram>
        gridFloorShader,
        ballShader,
        downsampleShader,
        cinematicShader,
        gaussianShader,
        bloomAccumulateShader;

    std::optional<GLMesh> gridFloor;
    std::optional<GLMesh> ball;

    // Relative window mouse position [-1, 1]
    std::optional<juce::Time> mouseEntered;
    juce::Point<float> mousePosition{ INITIAL_MOUSE };
    juce::Point<float> smoothMouse{ INITIAL_MOUSE };
};