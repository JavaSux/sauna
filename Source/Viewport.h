#pragma once

#include <JuceHeader.h>
#include <array>
#include <optional>

#include "util.h"

struct SaunaControls;

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


struct FaceCollector {
    std::vector<Vec3> vertices{};
    std::vector<GLuint> indices{};

    FaceCollector(FaceCollector &&other) noexcept {
		vertices = std::move(other.vertices);
		indices = std::move(other.indices);
    };
    FaceCollector() = default;

	FaceCollector const &operator=(FaceCollector const &) = delete;
	FaceCollector &operator=(FaceCollector &&) noexcept = default;

    std::span<Vec3, 3> getFace(size_t face) {
        auto offset = static_cast<size_t>(indices[face * 3]);
        return std::span<Vec3, 3>{ vertices.begin() + offset, 3 };
    }

    void addTriangle(
        Vec3 const &a,
        Vec3 const &b,
        Vec3 const &c
    ) {
        auto index{ static_cast<GLuint>(vertices.size()) };
        vertices.push_back(a);
		vertices.push_back(b);
		vertices.push_back(c);
        indices.push_back(index + 0);
        indices.push_back(index + 1);
        indices.push_back(index + 2);
    }

	std::vector<GLVertex> toVertices(juce::Colour const &color) const {
		std::array<float, 4> colorRaw{
			color.getFloatRed(),
			color.getFloatGreen(),
			color.getFloatBlue(),
			color.getFloatAlpha()
		};

		std::vector<GLVertex> result;
		result.reserve(vertices.size());

        auto faces = vertices.size() / 3;
        for (size_t face{ 0 }; face < faces; face++) {
            Vec3
                a{ vertices[face * 3 + 0] },
				b{ vertices[face * 3 + 1] },
				c{ vertices[face * 3 + 2] },
                normal{ (b - a).cross(c - a).normalized() };

            auto vertex{ [&](Vec3 const &position, std::array<float, 2> uv) -> GLVertex {
                return GLVertex{
                    .position = position.toArray(),
                    .normal = normal.toArray(),
                    .colour = colorRaw,
                    .texCoord = uv
                };
            }};

            result.push_back(vertex(a, { 0.0f, 0.0f }));
            result.push_back(vertex(b, { 1.0f, 0.0f }));
            result.push_back(vertex(c, { 0.0f, 1.0f }));
		}
		return result;
	}
};

struct GLMesh {
    bool owning;
    GLuint vertexBuffer, indexBuffer;
    GLsizei numIndices;

    GLMesh() = delete;
    GLMesh(GLMesh const &) = delete;
    GLMesh &operator=(GLMesh &) = delete;

    GLMesh(GLMesh &&other) noexcept {
        this->~GLMesh();

        owning = other.owning;
        vertexBuffer = other.vertexBuffer;
        indexBuffer = other.indexBuffer;
        numIndices = other.numIndices;

        other.owning = false;
    }

    GLMesh(std::span<const GLVertex> vertices, std::span<const GLuint> indices) :
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

    ~GLMesh() {
        if (owning) {
            juce::gl::glDeleteBuffers(1, &vertexBuffer);
            juce::gl::glDeleteBuffers(1, &indexBuffer);
        }
    }

    GLMesh &operator=(GLMesh &&other) noexcept {
        owning = other.owning;
        vertexBuffer = other.vertexBuffer;
        indexBuffer = other.indexBuffer;
        numIndices = other.numIndices;

        other.owning = false;

        return *this;
    }

    static GLMesh quad(juce::Colour const &color) {
        constexpr float SCALE = 1.0f;

        std::array<float, 4> colorRaw{
            color.getFloatRed(),
            color.getFloatGreen(),
            color.getFloatBlue(),
            color.getFloatAlpha()
        };
		auto vertex = [&colorRaw](std::array<float, 2> xy, std::array<float, 2> uv) {
			return GLVertex{
				.position = { xy[0] * SCALE, xy[1] * SCALE, 0.0f },
				.normal = { 0.0f, 0.0f, 1.0f },
				.colour = colorRaw,
				.texCoord = uv,
			};
		};
        std::vector<GLVertex> vertices{
            vertex({-1, -1}, {0.0, 0.0}),
			vertex({ 1, -1}, {1.0, 0.0}),
			vertex({-1,  1}, {0.0, 1.0}),
            vertex({ 1,  1}, {1.0, 1.0}),
        };
        std::vector<GLuint> indices{
            0, 1, 2,
            2, 1, 3
        };

        return { vertices, indices };
	}

    static GLMesh icosphere(int subdivisions, juce::Colour const &color) {
        // http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html

        std::vector<GLVertex> vertices;
        std::vector<GLuint> indices;

        // create 12 vertices of a icosahedron
        float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
        auto icosphere = FaceCollector{};

        std::array<Vec3, 12> planes = {
            Vec3{ -1,  t,  0 }, // 0
            Vec3{  1,  t,  0 }, // 1
            Vec3{ -1, -t,  0 }, // 2
            Vec3{  1, -t,  0 }, // 3

            Vec3{  0, -1,  t }, // 4
            Vec3{  0,  1,  t }, // 5
            Vec3{  0, -1, -t }, // 6
            Vec3{  0,  1, -t }, // 7

            Vec3{  t,  0, -1 }, // 8
            Vec3{  t,  0,  1 }, // 9
            Vec3{ -t,  0, -1 }, // 10
            Vec3{ -t,  0,  1 } // 11
        };

		std::array<int, 60> faces = {
			 0, 11,  5,
			 0,  5,  1,
			 0,  1,  7,
			 0,  7, 10,
			 0, 10, 11,
			 1,  5,  9,
			 5, 11,  4,
			11, 10,  2,
			10,  7,  6,
			 7,  1,  8,
			 3,  9,  4,
			 3,  4,  2,
			 3,  2,  6,
			 3,  6,  8,
			 3,  8,  9,
			 4,  9,  5,
			 2,  4, 11,
			 6,  2, 10,
			 8,  6,  7,
			 9,  8,  1,
		};

        for (int face{ 0 }; face < 20; face++) {
            icosphere.addTriangle(
                planes[faces[face * 3 + 0]].normalized(),
                planes[faces[face * 3 + 1]].normalized(),
                planes[faces[face * 3 + 2]].normalized()
            );
		}

        auto subdivided = FaceCollector{};
		for (int i{ 0 }; i < subdivisions; i++) {
			for (size_t face{ 0 }; face < icosphere.indices.size() / 3; face += 1) {
				auto face_ref = icosphere.getFace(face);
				Vec3
                    a{ face_ref[0] },
                    b{ face_ref[1] },
                    c{ face_ref[2] },
                    ab{ ((a + b) / 2.0f).normalized() },
                    bc{ ((b + c) / 2.0f).normalized() },
                    ca{ ((c + a) / 2.0f).normalized() };

				subdivided.addTriangle(a, ab, ca);
				subdivided.addTriangle(b, bc, ab);
				subdivided.addTriangle(c, ca, bc);
				subdivided.addTriangle(ab, bc, ca);
			}
			icosphere = std::move(subdivided);
            subdivided = FaceCollector{};
		}

		return { icosphere.toVertices(color), icosphere.indices };
    }

	void drawElements() const {
        juce::gl::glDrawElements(
            juce::gl::GL_TRIANGLES, 
            numIndices,
            juce::gl::GL_UNSIGNED_INT, 
            nullptr
        );
    }
};


struct GLMeshObject {
    GLMesh mesh;
    std::shared_ptr<juce::OpenGLShaderProgram> shader;
    GLVertexAttributes attribs;
    GLMeshUniforms uniforms;
    juce::Matrix3D<float> modelMatrix;

    GLMeshObject() = delete;
    GLMeshObject(GLMeshObject const &) = delete;
    GLMeshObject &operator=(GLMeshObject const &) = delete;

    GLMeshObject(
        GLMesh &&handle,
        std::shared_ptr<juce::OpenGLShaderProgram> &shader,
        juce::Matrix3D<float> modelMatrix = {}
    ) noexcept : 
        mesh{ std::move(handle) },
        attribs{ *shader },
        uniforms{ *shader },
        shader{ shader },
        modelMatrix{ modelMatrix }
    {}
    GLMeshObject(GLMeshObject &&) noexcept = default;
    GLMeshObject &operator=(GLMeshObject &&) noexcept = default;
    ~GLMeshObject() = default;
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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

    GLMesh fullscreenQuad;

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
        fullscreenQuad{ GLMesh::quad(juce::Colours::black) },
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

    ViewportComponent(SaunaControls const &pluginState) :
		pluginState{ pluginState },
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
	SaunaControls const &pluginState;
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
        bloomAccumulateShader,
        icosphereShader;

    std::optional<GLMeshObject>
        gridFloor,
        ball,
        icosphere;

    // Relative window mouse position [-1, 1]
    std::optional<juce::Time> mouseEntered;
    juce::Point<float> mousePosition{ INITIAL_MOUSE };
    juce::Point<float> smoothMouse{ INITIAL_MOUSE };
};