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

Uniforms::Uniforms(juce::OpenGLShaderProgram const &shader) {
    auto tryEmplace{ [&](std::optional<Uniform> &location, GLchar const *name) {
        if (juce::gl::glGetUniformLocation(shader.getProgramID(), name) >= 0) {
            location.emplace(shader, name);
        } else {
            location.reset();
        }
    } };

    tryEmplace(projectionMatrix, "projectionMatrix");
    tryEmplace(viewMatrix, "viewMatrix");
    tryEmplace(modelMatrix, "modelMatrix");
}


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
}

BufferHandle::BufferHandle(BufferHandle &&other) noexcept :
    owning{ other.owning },
    vertexBuffer{ other.vertexBuffer },
    indexBuffer{ other.indexBuffer },
    numIndices{ other.numIndices }
{
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

BufferHandle BufferHandle::quad(float size, float z, juce::Colour const &color) {
    float scale = size / 2.0;

    std::array<float, 4> colorRaw{
        color.getFloatRed(),
        color.getFloatGreen(),
        color.getFloatBlue(),
        color.getFloatAlpha()
    };
    std::vector<Vertex> vertices{
        Vertex{
            .position = { -scale, -scale, z },
            .colour = colorRaw,
            .texCoord = { 0.0, 0.0 }
        },
        Vertex{
            .position = { scale, -scale, z },
            .colour = colorRaw,
            .texCoord = { 1.0, 0.0 }
        },
        Vertex{
            .position = { -scale, scale, z },
            .colour = colorRaw,
            .texCoord = { 0.0, 1.0 }
        },
        Vertex{
            .position = { scale, scale, z },
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


const juce::Point<float> ViewportRenderer::INITIAL_MOUSE{ 0.5f, 0.6f };
const juce::Colour ViewportRenderer::CLEAR_COLOR = juce::Colour::fromFloatRGBA(0.05f, 0.05f, 0.05f, 1.0f);

ViewportRenderer::ViewportRenderer() :
    juce::OpenGLAppComponent{},
    gridFloorShader{ nullptr },
    startTime{ juce::Time::getCurrentTime() },
    lastUpdateTime{ startTime },
    vBlankTimer{ this, [this](){ update(); } }
{}

ViewportRenderer::~ViewportRenderer() {
    shutdownOpenGL();
}

void ViewportRenderer::initialise() {
    // May be called mulitple times by the parent
    DBG("Initializing OpenGL resources");

    juce::String
        standardVS{ juce::OpenGLHelpers::translateVertexShaderToV3(BinaryData::standard_vert_glsl) },
        gridFloorFS{ juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::gridfloor_frag_glsl) },
        ballFS{ juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::ball_frag_glsl) };

    gridFloorShader = std::make_shared<juce::OpenGLShaderProgram>(openGLContext);
    if (
        !gridFloorShader->addVertexShader(standardVS)
        || !gridFloorShader->addFragmentShader(gridFloorFS)
        || !gridFloorShader->link()
    ) {
        throw std::runtime_error{ gridFloorShader->getLastError().toStdString() };
    }

    gridFloorShader->use();

    gridFloor.emplace(
        BufferHandle::quad(6.0, 0.0, juce::Colour::fromHSL(0.1f, 0.6f, 0.57f, 1.0f)),
        gridFloorShader
    );

    ballShader = std::make_shared<juce::OpenGLShaderProgram>(openGLContext);
    if (
        !ballShader->addVertexShader(standardVS)
        || !ballShader->addFragmentShader(ballFS)
        || !ballShader->link()
        ) {
        throw std::runtime_error{ ballShader->getLastError().toStdString() };
    }

    ballShader->use();

    ball.emplace(
        BufferHandle::quad(0.3, 0.1, juce::Colour::fromHSL(0.6f, 0.6f, 0.57f, 1.0f)),
        ballShader
    );
}

// Called by `vBlankTimer`
void ViewportRenderer::update() {
    repaint(); // Request
    juce::Time now = juce::Time::getCurrentTime();

    float delta = static_cast<float>((now - lastUpdateTime).inSeconds());
    float elapsed = static_cast<float>((now - startTime).inSeconds());
    smoothMouse = expEase(smoothMouse, mousePosition, 16.0, delta);

    if (ball)
        ball->modelMatrix = juce::Matrix3D<float>::fromTranslation({
            std::sin(elapsed),
            std::cos(elapsed),
            std::sin(elapsed * 4.0f)
        });

    lastUpdateTime = now;
}

void ViewportRenderer::render() {
    jassert(juce::OpenGLHelpers::isContextActive());

    float desktopScale{ static_cast<float>(openGLContext.getRenderingScale()) };

    juce::Matrix3D<float> projectionMatrix{ [this]() {
        float halfWidth = 0.5f;
        float halfHeight = halfWidth * getLocalBounds().toFloat().getAspectRatio(false);

        return juce::Matrix3D<float>::fromFrustum(
            -halfWidth, halfWidth,
            -halfHeight, halfHeight,
            1.0f, 10.0f
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

        if (mesh.uniforms.projectionMatrix) {
            mesh.uniforms.projectionMatrix->setMatrix4(projectionMatrix.mat, 1, false);
        }
        if (mesh.uniforms.viewMatrix) {
            mesh.uniforms.viewMatrix->setMatrix4(viewMatrix.mat, 1, false);
        }
        if (mesh.uniforms.modelMatrix) {
            mesh.uniforms.modelMatrix->setMatrix4(mesh.modelMatrix.mat, 1, false);
        }

        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, mesh.bufferHandle.vertexBuffer);
        juce::gl::glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, mesh.bufferHandle.indexBuffer);

        mesh.attribs.enable();
        juce::gl::glDrawElements(juce::gl::GL_TRIANGLES, mesh.bufferHandle.numIndices, juce::gl::GL_UNSIGNED_INT, nullptr);
        mesh.attribs.disable();

        opengl_assert();
    } };

    juce::gl::glDepthMask(juce::gl::GL_TRUE);
    juce::gl::glEnable(juce::gl::GL_DEPTH_TEST);
    // Clear frame, depth, stencil
    juce::OpenGLHelpers::clear(CLEAR_COLOR);

    draw(ball.value());

    // Additive rendering
    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_ONE, juce::gl::GL_ONE);
    juce::gl::glBlendEquation(juce::gl::GL_FUNC_ADD);

    // Read-only depth buffer for transparent elements
    juce::gl::glDepthMask(juce::gl::GL_FALSE);
    draw(gridFloor.value());

    // Reset the element buffers so child Components draw correctly
    juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
    opengl_assert();
}

void ViewportRenderer::paint(juce::Graphics &) {
    // Draw overtop of the OpenGL render
}

void ViewportRenderer::shutdown() {
    // May be called multiple times by the parent
    gridFloor.reset();
    gridFloorShader.reset();
}

void ViewportRenderer::mouseMove(juce::MouseEvent const &event) {
    auto bounds = getLocalBounds().toFloat();
    mousePosition = event.position / juce::Point<float>{ bounds.getWidth(), bounds.getHeight() };
}

void ViewportRenderer::mouseExit(juce::MouseEvent const &) {
    mousePosition = INITIAL_MOUSE;
}

SaunaEditor::SaunaEditor(SaunaProcessor &p) :
    AudioProcessorEditor{ &p },
    audioProcessor{ p },
    viewport{}
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(800, 600);
    setTitle("Sauna");
    addAndMakeVisible(viewport);
}

SaunaEditor::~SaunaEditor() {}

void SaunaEditor::paint(juce::Graphics &g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SaunaEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor
    
    viewport.setBounds(10, 10, 780, 450);
    // TODO make responsive layout
    // TODO https://docs.juce.com/master/classLowLevelGraphicsContext.html#a088c81d6d2bff0f952f990e7f673f020
    // TODO https://docs.juce.com/master/classPath.html#a501f83b0e323fe86d33c047f83451065 
}
