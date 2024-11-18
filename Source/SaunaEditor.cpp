#include "SaunaProcessor.h"
#include "SaunaEditor.h"

#include <string_view>
#include <stdint.h>
#include "util.h"

VertexAttributes::VertexAttributes(juce::OpenGLShaderProgram const &shader) {
    auto tryEmplace{ [&](std::optional<Attribute> &location, GLchar const *name) {
        if (juce::gl::glGetAttribLocation(shader.getProgramID(), name) >= 0) {
            location.emplace(shader, name);
        } else {
            location.reset();
        }
    }};

    tryEmplace(position, "position");
    tryEmplace(normal, "normal");
    tryEmplace(sourceColour, "sourceColour");
    tryEmplace(textureCoordIn, "textureCoordIn");
}

void VertexAttributes::enable() const {
    size_t offset{ 0 };

    auto vertexBuffer{ [&](std::optional<juce::OpenGLShaderProgram::Attribute> const &attrib, GLint size) {
        if (attrib) {
            juce::gl::glVertexAttribPointer(
                attrib->attributeID, size,
                juce::gl::GL_FLOAT, juce::gl::GL_FALSE,
                sizeof(Vertex), reinterpret_cast<GLvoid *>(sizeof(float) * offset)
            );
            juce::gl::glEnableVertexAttribArray(attrib->attributeID);
        }

        offset += size;
    } };

    vertexBuffer(position, 3);
    vertexBuffer(normal, 3);
    vertexBuffer(sourceColour, 4);
    vertexBuffer(textureCoordIn, 2);
}

void VertexAttributes::disable() const {
    auto disable{ [](std::optional<Attribute> const &attrib) {
        if (attrib) {
            juce::gl::glDisableVertexAttribArray(attrib->attributeID);
        }
    } };

    disable(position);
    disable(normal);
    disable(sourceColour);
    disable(textureCoordIn);
}

MeshUniforms::MeshUniforms(juce::OpenGLShaderProgram const &shader) :
	modelMatrix{ shader, "modelMatrix" },
	viewMatrix{ shader, "viewMatrix" },
	projectionMatrix{ shader, "projectionMatrix" }
{}


BufferHandle::BufferHandle(
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
	OPENGL_ASSERT();
}

BufferHandle::BufferHandle(BufferHandle &&other) noexcept {
    this->~BufferHandle();
    
	owning = other.owning;
	vertexBuffer = other.vertexBuffer;
	indexBuffer = other.indexBuffer;
	numIndices = other.numIndices;

    other.owning = false;
}

BufferHandle &BufferHandle::operator=(BufferHandle &&other) noexcept {
    owning = other.owning;
    vertexBuffer = other.vertexBuffer;
    indexBuffer = other.indexBuffer;
    numIndices = other.numIndices;

    other.owning = false;

    return *this;
}

BufferHandle BufferHandle::quad(float size, juce::Colour const &color) {
    float scale = size / 2.0f;

    std::array<float, 4> colorRaw{
        color.getFloatRed(),
        color.getFloatGreen(),
        color.getFloatBlue(),
        color.getFloatAlpha()
    };
    std::vector<Vertex> vertices{
        Vertex{
            .position = { -scale, -scale, 0.0f },
            .colour = colorRaw,
            .texCoord = { 0.0, 0.0 }
        },
        Vertex{
            .position = { scale, -scale, 0.0f },
            .colour = colorRaw,
            .texCoord = { 1.0, 0.0 }
        },
        Vertex{
            .position = { -scale, scale, 0.0f },
            .colour = colorRaw,
            .texCoord = { 0.0, 1.0 }
        },
        Vertex{
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

const juce::Point<float> ViewportComponent::INITIAL_MOUSE{ 0.5f, 0.6f };
const juce::Colour ViewportComponent::CLEAR_COLOR = juce::Colours::black;
const int ViewportComponent::SUPERSAMPLE = 3;
const double ViewportComponent::MOUSE_DELAY = 0.4;

ViewportComponent::ViewportComponent() :
    juce::OpenGLAppComponent{},
    gridFloorShader{ nullptr },
    startTime{ juce::Time::getCurrentTime() },
    lastUpdateTime{ startTime },
    vBlankTimer{ this, [this](){ update(); } }
{}

ViewportComponent::~ViewportComponent() {
    shutdownOpenGL();
}

void ViewportComponent::initialise() {
    // May be called mulitple times by the parent
    DBG("Initializing ViewportComponent resources");

    juce::String
        standardVS   { juce::OpenGLHelpers::translateVertexShaderToV3(BinaryData::standard_vert_glsl) },
        billboardVS  { juce::OpenGLHelpers::translateVertexShaderToV3(BinaryData::billboard_vert_glsl) },
        postprocessVS{ juce::OpenGLHelpers::translateVertexShaderToV3(BinaryData::postprocess_vert_glsl) },
        gridFloorFS  { juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::gridfloor_frag_glsl) },
        ballFS       { juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::ball_frag_glsl) },
		downsampleFS { juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::downsample_frag_glsl) },
        cinematicFS  { juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::cinematic_frag_glsl) };

    recomputeViewportSize();

	gridFloorShader = loadShader(openGLContext, standardVS, gridFloorFS, "gridFloorShader");
    gridFloor.emplace(
        BufferHandle::quad(6.0, juce::Colour::fromHSL(0.1f, 0.73f, 0.55f, 1.0f)),
        gridFloorShader
    );


    ballShader = loadShader(openGLContext, billboardVS, ballFS, "ballShader");
    ball.emplace(
        BufferHandle::quad(0.5f, juce::Colours::white),
        ballShader
    );


	downsampleShader = loadShader(openGLContext, postprocessVS, downsampleFS, "downsampleShader");
	cinematicShader = loadShader(openGLContext, postprocessVS, cinematicFS, "cinematicShader");
    postprocess.emplace(
        downsampleShader,
        cinematicShader,
        juce::Point<int>{ componentBounds.getWidth(), componentBounds.getHeight() }, 
        ViewportComponent::SUPERSAMPLE
    );
}

void ViewportComponent::recomputeViewportSize() {
    componentBounds = { getLocalBounds() * openGLContext.getRenderingScale() };
    renderBounds = { componentBounds * SUPERSAMPLE };
	// Cannot resize postprocess buffers here because OpenGL context is not active in `resized`
}

// Called by `vBlankTimer`
void ViewportComponent::update() {
    repaint(); // Request
    juce::Time now = juce::Time::getCurrentTime();

    if (
        (smoothMouse - INITIAL_MOUSE).getDistanceFromOrigin() > 0.0001
        || mouseEntered && (now - *mouseEntered).inSeconds() > MOUSE_DELAY
    ) {
        float delta = static_cast<float>((now - lastUpdateTime).inSeconds());
        smoothMouse = expEase(smoothMouse, mousePosition, 16.0, delta);
    }

    float elapsed = static_cast<float>((now - startTime).inSeconds());
    if (ball)
        ball->modelMatrix = juce::Matrix3D<float>::fromTranslation({
            std::sin(elapsed / 4.0f),
            std::cos(elapsed / 4.0f),
            0.0f
        });

    lastUpdateTime = now;
}

void ViewportComponent::render() {
    jassert(juce::OpenGLHelpers::isContextActive());
    jassert(postprocess);
	jassert(gridFloor);
	jassert(ball);

    juce::Matrix3D<float> projectionMatrix{ [this]() {
        float halfWidth = 0.25f;
        float halfHeight = halfWidth * getLocalBounds().toFloat().getAspectRatio(false);

        return juce::Matrix3D<float>::fromFrustum(
            -halfWidth, halfWidth,
            -halfHeight, halfHeight,
            0.5f, 10.0f
        );
    }() };

    juce::Matrix3D<float> viewMatrix{ [this]() {
        const float pi = juce::MathConstants<float>::pi;
        const float pi_2 = pi / 2.0;

        juce::Matrix3D<float> radius = juce::Matrix3D<float>::fromTranslation({ 0.0f, 0.0f, -5.0f });
        juce::Matrix3D<float> pivot = radius.rotation({
            // altitude
            (1.0f - smoothMouse.y) * -pi,
            0.0f,
            // Turntable
            (smoothMouse.x * 2.0f + 1.0f) * pi_2
            });
        juce::Matrix3D<float> lift = juce::Matrix3D<float>::fromTranslation({ 0.0f, 0.0f, -0.25f });
        return radius * pivot * lift;
    }() };

    const auto draw{ [&projectionMatrix, &viewMatrix](Mesh const &mesh) {
        mesh.shader->use();

        if (mesh.uniforms.projectionMatrix.uniformID >= 0) {
            mesh.uniforms.projectionMatrix.setMatrix4(projectionMatrix.mat, 1, false);
        }
        if (mesh.uniforms.viewMatrix.uniformID >= 0) {
            mesh.uniforms.viewMatrix.setMatrix4(viewMatrix.mat, 1, false);
        }
        if (mesh.uniforms.modelMatrix.uniformID >= 0) {
            mesh.uniforms.modelMatrix.setMatrix4(mesh.modelMatrix.mat, 1, false);
        }

        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, mesh.bufferHandle.vertexBuffer);
        juce::gl::glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, mesh.bufferHandle.indexBuffer);

        mesh.attribs.enable();
        juce::gl::glDrawElements(juce::gl::GL_TRIANGLES, mesh.bufferHandle.numIndices, juce::gl::GL_UNSIGNED_INT, nullptr);
        mesh.attribs.disable();
    } };

	postprocess->sizeTo({ componentBounds.getWidth(), componentBounds.getHeight() }, SUPERSAMPLE);


    /* ===================================== */
    /* Scene rendering */
    
	postprocess->rasterBuffer.bindFramebuffer();
    juce::gl::glViewport(
        renderBounds.getX(),     renderBounds.getY(), 
        renderBounds.getRight(), renderBounds.getBottom()
    );
    
    juce::gl::glDepthMask(juce::gl::GL_TRUE);
    juce::gl::glEnable(juce::gl::GL_DEPTH_TEST);

    // Clear frame, depth, stencil. Must happen after re-enabling depth mask
    juce::OpenGLHelpers::clear(CLEAR_COLOR);

    draw(ball.value());

    // Additive rendering
    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_ONE, juce::gl::GL_ONE);
    juce::gl::glBlendEquation(juce::gl::GL_FUNC_ADD);

    // Read-only depth buffer for transparent elements
    juce::gl::glDepthMask(juce::gl::GL_FALSE);

    draw(gridFloor.value());


    /* ===================================== */
    /* Postprocess downsample */
	postprocess->downsampleBuffer.bindFramebuffer();
    juce::gl::glViewport(
        componentBounds.getX(),     componentBounds.getY(), 
        componentBounds.getRight(), componentBounds.getBottom()
    );

    juce::gl::glDisable(juce::gl::GL_DEPTH_TEST);
    juce::gl::glDisable(juce::gl::GL_BLEND);

	postprocess->downsampleShader->use();
	postprocess->setDownsampleUniforms(ViewportComponent::SUPERSAMPLE);

	juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, postprocess->fullscreenQuad.vertexBuffer);
	juce::gl::glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, postprocess->fullscreenQuad.indexBuffer);

	postprocess->downsampleAttribs.enable();
    // juce::gl::glActiveTexture(GL_TEXTURE0); // not necessary because texture 0 is active by default
    juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, postprocess->rasterBuffer.outputTexture);
	juce::gl::glDrawElements(juce::gl::GL_TRIANGLES, postprocess->fullscreenQuad.numIndices, juce::gl::GL_UNSIGNED_INT, nullptr);
	postprocess->downsampleAttribs.disable();


    /* Postprocess cinematic FX */
    juce::gl::glBindFramebuffer(juce::gl::GL_FRAMEBUFFER, 0);
	postprocess->cinematicShader->use();
	postprocess->setCinematicUniforms();

	juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, postprocess->fullscreenQuad.vertexBuffer);
	juce::gl::glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, postprocess->fullscreenQuad.indexBuffer);

	postprocess->cinematicAttribs.enable();
	// juce::gl::glActiveTexture(GL_TEXTURE0); // not necessary because texture 0 is active by default
	juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, postprocess->downsampleBuffer.outputTexture);
	juce::gl::glDrawElements(juce::gl::GL_TRIANGLES, postprocess->fullscreenQuad.numIndices, juce::gl::GL_UNSIGNED_INT, nullptr);
	postprocess->cinematicAttribs.disable();


    // Reset the element buffers so child Components draw correctly
    juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
}

void ViewportComponent::resized() {
	recomputeViewportSize();
}

void ViewportComponent::shutdown() {
    // May be called multiple times by the parent
    gridFloor.reset();
    gridFloorShader.reset();
	ball.reset();
	ballShader.reset();
	postprocess.reset();
	downsampleShader.reset();
	cinematicShader.reset();
}

void ViewportComponent::mouseMove(juce::MouseEvent const &event) {
    auto bounds = getLocalBounds().toFloat();
    mousePosition = event.position / juce::Point<float>{ bounds.getWidth(), bounds.getHeight() };
}

void ViewportComponent::mouseEnter(juce::MouseEvent const &) {
    mouseEntered.emplace(juce::Time::getCurrentTime());
}

void ViewportComponent::mouseExit(juce::MouseEvent const &) {
    mouseEntered.reset();
    mousePosition = INITIAL_MOUSE;
}


ControlPanelComponent::ControlPanelComponent(SaunaProcessor &p) : 
    saunaProcessor{ p },
    dropShadow{ juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.8f), 10, {} }
{
    leftPanel.setColour(juce::LookAndFeel_V4::ColourScheme::UIColour::widgetBackground, juce::Colours::red);
    rightPanel.setColour(juce::LookAndFeel_V4::ColourScheme::UIColour::widgetBackground, juce::Colours::blue);

    addAndMakeVisible(leftPanel);
    addAndMakeVisible(rightPanel);
}

void ControlPanelComponent::resized() {
    auto bounds = getLocalBounds();
    leftPanel.setBounds(bounds.removeFromLeft(200).reduced(8));
    rightPanel.setBounds(bounds.reduced(8));
}

void ControlPanelComponent::paint(juce::Graphics &graphics) {
	graphics.fillAll(juce::Colour::fromHSL(0.0f, 0.0f, 0.15f, 1.0));
    // Fake shadow of viewport frame
    dropShadow.drawForRectangle(graphics, { 0, -10, getWidth(), 10 });
}


SaunaEditor::SaunaEditor(SaunaProcessor &p) :
    AudioProcessorEditor{ &p },
    audioProcessor{ p },
    viewport{ viewportFrame.viewport },
	controlPanel{ p },
    constrainer{},
    resizer{ this, &constrainer }
{
    setSize(800, 600);
    setTitle("Sauna");

    addAndMakeVisible(viewportFrame);
    addAndMakeVisible(controlPanel);
    resizer.setAlwaysOnTop(true);
    addAndMakeVisible(resizer);
    constrainer.setMinimumSize(300, 250);
}

void SaunaEditor::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour::fromHSL(0.0f, 0.0f, 0.28f, 1.0f));
}

void SaunaEditor::resized() {
	resizer.setBounds(getWidth() - 16, getHeight() - 16, 16, 16);

	auto bounds = getLocalBounds();
	controlPanel.setBounds(bounds.removeFromBottom(120));
	viewportFrame.setBounds(bounds);
}
