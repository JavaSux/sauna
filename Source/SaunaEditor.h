#pragma once

#include <JuceHeader.h>
#include "SaunaProcessor.h"

const juce::Colour ACCENT_COLOR = juce::Colour::fromHSL(0.11f, 1.0f, 0.35f, 1.0);

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


struct MeshUniforms {
    using Uniform = juce::OpenGLShaderProgram::Uniform;

    // May be nullopt for a given shaderprogram if the uniform gets pruned
    juce::OpenGLShaderProgram::Uniform
        modelMatrix,
        viewMatrix,
        projectionMatrix;

    MeshUniforms() = delete;
    MeshUniforms(juce::OpenGLShaderProgram const &);
	MeshUniforms(MeshUniforms const &) = delete;
	MeshUniforms(MeshUniforms &&) noexcept = default;
	~MeshUniforms() = default;
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

    static BufferHandle quad(float scale, juce::Colour const &color);

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
    MeshUniforms uniforms;
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


struct BackBuffer {
    bool owning{ true };
    GLuint frameBuffer{ 0 }, outputTexture{ 0 }, depthStencilBuffer{ 0 };
    juce::Point<int> resolution;

    BackBuffer(BackBuffer const &) = delete;
    BackBuffer &operator=(BackBuffer const &) = delete;
    BackBuffer(BackBuffer &&other) noexcept {
        this->~BackBuffer();

		owning = other.owning;
		frameBuffer = other.frameBuffer;
		outputTexture = other.outputTexture;
		depthStencilBuffer = other.depthStencilBuffer;

		other.owning = false;
    };
	BackBuffer &operator=(BackBuffer &&other) noexcept {
		this->~BackBuffer();

		owning = other.owning;
		frameBuffer = other.frameBuffer;
		outputTexture = other.outputTexture;
		depthStencilBuffer = other.depthStencilBuffer;
		other.owning = false;
		return *this;
    }

    BackBuffer(juce::Point<int> resolution) : resolution{ resolution } {
		using namespace juce::gl;

		glGenFramebuffers(1, &frameBuffer);
        OPENGL_ASSERT();
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        OPENGL_ASSERT();

		// Use texture for color info
		glGenTextures(1, &outputTexture);
		glBindTexture(GL_TEXTURE_2D, outputTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, resolution.x, resolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        OPENGL_ASSERT();

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
        OPENGL_ASSERT();

        // Use renderbuffer for depth/stencil because they are not needed in the shader
		glGenRenderbuffers(1, &depthStencilBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthStencilBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, resolution.x, resolution.y);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        OPENGL_ASSERT();

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilBuffer);
		jassert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void bindFramebuffer() const {
		juce::gl::glBindFramebuffer(juce::gl::GL_FRAMEBUFFER, frameBuffer);
	}

	~BackBuffer() {
        if (owning) {
            juce::gl::glDeleteTextures(1, &outputTexture);
            juce::gl::glDeleteRenderbuffers(1, &depthStencilBuffer);
            juce::gl::glDeleteFramebuffers(1, &frameBuffer);
        }
	}
};

struct PostProcess {
    using Uniform = juce::OpenGLShaderProgram::Uniform;

    BufferHandle fullscreenQuad;
    VertexAttributes downsampleAttribs, cinematicAttribs;
    std::shared_ptr<juce::OpenGLShaderProgram> downsampleShader, cinematicShader;
    BackBuffer rasterBuffer, downsampleBuffer;

    Uniform supersampleUniform, renderedImageUniform, downsampledImageUniform;

    PostProcess(PostProcess const &) = delete;
    PostProcess(PostProcess &&) noexcept = default;
    PostProcess &operator=(PostProcess const &) = delete;
    PostProcess &operator=(PostProcess &&) noexcept = default;

    PostProcess(
        std::shared_ptr<juce::OpenGLShaderProgram> &downsampleShader,
        std::shared_ptr<juce::OpenGLShaderProgram> &cinematicShader,
        juce::Point<int> originalResolution,
        int supersample
    ) noexcept :
        fullscreenQuad{ BufferHandle::quad(2.0, juce::Colours::black) },
        downsampleAttribs{ *downsampleShader },
        cinematicAttribs{ *cinematicShader },
        supersampleUniform{ *downsampleShader, "supersample" },
        renderedImageUniform{ *downsampleShader, "renderedImage" },
        downsampledImageUniform{ *cinematicShader, "downsampledImage" },
        downsampleShader{ downsampleShader },
        cinematicShader{ cinematicShader },
        rasterBuffer{ originalResolution * supersample },
        downsampleBuffer{ originalResolution }
    {}

    ~PostProcess() = default;

    void setDownsampleUniforms(int supersample) const {
        if (supersampleUniform.uniformID >= 0) {
            supersampleUniform.set(supersample);
        }

        if (renderedImageUniform.uniformID >= 0) {
            renderedImageUniform.set(0); // GL_TEXTURE0
        }
    }

    void setCinematicUniforms() const {
        if (downsampledImageUniform.uniformID >= 0) {
            downsampledImageUniform.set(0); // GL_TEXTURE0
        }
    }

    void sizeTo(juce::Point<int> viewportSize, int supersample) {
        if (rasterBuffer.resolution != viewportSize * supersample) {
            rasterBuffer = BackBuffer{ viewportSize * supersample };
        }

		if (downsampleBuffer.resolution != viewportSize) {
			downsampleBuffer = BackBuffer{ viewportSize };
		}
    }
};


struct ViewportComponent: juce::OpenGLAppComponent {
    static const juce::Point<float> INITIAL_MOUSE;
    static const juce::Colour CLEAR_COLOR;
    static const int SUPERSAMPLE;
    static const double MOUSE_DELAY;

    ViewportComponent();
    ViewportComponent(ViewportComponent const &) = delete;
    ViewportComponent &operator=(ViewportComponent const &) = delete;
    ~ViewportComponent();

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
    juce::Time lastUpdateTime;
    juce::Time startTime;

    juce::OpenGLContext openGLContext;
	juce::Rectangle<int> componentBounds, renderBounds;
	std::optional<PostProcess> postprocess;

    std::shared_ptr<juce::OpenGLShaderProgram> gridFloorShader;
    std::shared_ptr<juce::OpenGLShaderProgram> ballShader;
    std::shared_ptr<juce::OpenGLShaderProgram> downsampleShader;
	std::shared_ptr<juce::OpenGLShaderProgram> cinematicShader;

    std::optional<Mesh> gridFloor;
    std::optional<Mesh> ball;

    // Relative window mouse position [-1, 1]
    std::optional<juce::Time> mouseEntered;
    juce::Point<float> mousePosition{ INITIAL_MOUSE };
    juce::Point<float> smoothMouse{ INITIAL_MOUSE };
};

struct ViewportFrameComponent: juce::Component {
    juce::Path roundRectPath;
    juce::DropShadow dropShadow;
    ViewportComponent viewport;

    ViewportFrameComponent() :
        dropShadow{ juce::Colour::fromFloatRGBA(0.0, 0.0, 0.0, 0.6), 4, { 0, 2 } }
    {
        addAndMakeVisible(viewport);
        roundRectPath.addRoundedRectangle(0, 0, getHeight(), getHeight() , 3);
    }

	void resized() override {
        auto bounds = getLocalBounds().reduced(16);
		viewport.setBounds(bounds);

		roundRectPath.clear();
		roundRectPath.addRoundedRectangle(
            bounds.getX() - 1, bounds.getY() - 1, 
            bounds.getWidth() + 2, bounds.getHeight() + 2, 
            3
        );
	}

    // Can't draw border over OpenGL because OpenGL is forced to render on top of everything else
    void paint(juce::Graphics &graphics) override {
        dropShadow.drawForPath(graphics, roundRectPath);
		graphics.setColour(ACCENT_COLOR);
		graphics.strokePath(roundRectPath, juce::PathStrokeType(2.0f));
    }
};

struct ControlPanelComponent: juce::Component {
    SaunaProcessor &saunaProcessor;
    juce::Component leftPanel, rightPanel;
    juce::DropShadow dropShadow;

    ControlPanelComponent() = delete;
    ControlPanelComponent(SaunaProcessor &p);
    ControlPanelComponent(ControlPanelComponent const &) = delete;
    ControlPanelComponent &operator=(ControlPanelComponent const &) = delete;
    ~ControlPanelComponent() override = default;

    void paint(juce::Graphics &) override;
    void resized() override;
};

struct SaunaEditor: juce::AudioProcessorEditor {
    SaunaEditor(SaunaProcessor&);
    SaunaEditor(const SaunaEditor&) = delete;
    SaunaEditor &operator=(SaunaEditor const &) = delete;
    ~SaunaEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SaunaProcessor &audioProcessor;
	juce::ComponentBoundsConstrainer constrainer;
    juce::ResizableCornerComponent resizer;
	ViewportFrameComponent viewportFrame;
    ViewportComponent &viewport;
	ControlPanelComponent controlPanel;

    JUCE_LEAK_DETECTOR(SaunaEditor)
};
