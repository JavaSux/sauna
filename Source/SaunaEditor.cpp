#include "SaunaProcessor.h"
#include "SaunaEditor.h"

#include <string_view>
#include <stdint.h>
#include "util.h"

static void tryEmplaceUniform(
    std::optional<juce::OpenGLShaderProgram::Uniform> &location,
    juce::OpenGLShaderProgram const &shader,
    char const *name
) {
    if (juce::gl::glGetUniformLocation(shader.getProgramID(), name) >= 0) {
        location.emplace(shader, name);
    } else {
        location.reset();
        DBG("No uniform found for " << name);
    }
}

static void tryEmplaceAttribute(
    std::optional<juce::OpenGLShaderProgram::Attribute> &location,
    juce::OpenGLShaderProgram const &shader,
    char const *name
) {
    if (juce::gl::glGetAttribLocation(shader.getProgramID(), name) >= 0) {
        location.emplace(shader, name);
    } else {
        location.reset();
        DBG("No attribute found for " << name);
    }
}

VertexAttributes::VertexAttributes(juce::OpenGLShaderProgram &shader) {
    tryEmplaceAttribute(position, shader, "position");
    tryEmplaceAttribute(normal, shader, "normal");
    tryEmplaceAttribute(sourceColour, shader, "sourceColour");
    tryEmplaceAttribute(textureCoordIn, shader, "textureCoordIn");
}

void VertexAttributes::enable() const {
    size_t offset{ 0 };

    auto vertexBuffer = [&](decltype(position) const &attrib, GLint size) {
        if (attrib) {
            juce::gl::glVertexAttribPointer(
                attrib->attributeID,
                size,
                juce::gl::GL_FLOAT,
                juce::gl::GL_FALSE,
                sizeof(Vertex),
                reinterpret_cast<GLvoid *>(sizeof(float) * offset)
            );
            juce::gl::glEnableVertexAttribArray(attrib->attributeID);
        }

        offset += size;
    };

    vertexBuffer(position, 3);
    vertexBuffer(normal, 3);
    vertexBuffer(sourceColour, 4);
    vertexBuffer(textureCoordIn, 2);
}

void VertexAttributes::disable() const {
    auto disable = [](decltype(position) const &attrib) {
        if (attrib) juce::gl::glDisableVertexAttribArray(attrib->attributeID);
    };

    disable(position);
    disable(normal);
    disable(sourceColour);
    disable(textureCoordIn);
}


void Mesh::pushQuad(float size, std::array<float, 4> const &color) {
    float scale = size / 2.0;

    std::vector<Vertex> vertices{
        Vertex{
            .position = { -scale, -scale, 0.0 },
            .colour = color,
            .texCoord = { 0.0, 0.0 }
        },
        Vertex{
            .position = { scale, -scale, 0.0 },
            .colour = color,
            .texCoord = { 1.0, 0.0 }
        },
        Vertex{
            .position = { -scale, scale, 0.0 },
            .colour = color,
            .texCoord = { 0.0, 1.0 }
        },
        Vertex{
            .position = { scale, scale, 0.0 },
            .colour = color,
            .texCoord = { 1.0, 1.0 }
        }
    };
    std::vector<GLuint> indices{
        0, 1, 2,
        2, 1, 3
    };

    bufferHandles.emplace_back(vertices, indices);
}

const juce::Point<float> ViewportRenderer::INITIAL_MOUSE{ 0.5f, 0.6f };

ViewportRenderer::ViewportRenderer() :
    juce::OpenGLAppComponent{},
    shaderProgram{openGLContext},
    lastUpdateTime{ juce::Time::getCurrentTime() },
    vBlankTimer{ this, [this] { update(); } }
{}

ViewportRenderer::~ViewportRenderer() {
    shutdownOpenGL();
}

void ViewportRenderer::initialise() {
    // May be called mulitple times by the parent
    DBG("Initializing shaders");
    shaderProgram.release();

    if (
        !shaderProgram.addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(BinaryData::standard_vert_glsl))
        || !shaderProgram.addFragmentShader(juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::gridfloor_frag_glsl))
        || !shaderProgram.link()
    ) {
        throw std::runtime_error{ shaderProgram.getLastError().toStdString() };
    }

    shaderProgram.use();

    mesh = Mesh{};
    mesh.pushQuad(6.0, { 1.0, 1.0, 1.0, 1.0 });

    vertexAttributes.emplace(shaderProgram);
    tryEmplaceUniform(projectionMatrixUniform, shaderProgram, "projectionMatrix");
    tryEmplaceUniform(viewMatrixUniform, shaderProgram, "viewMatrix");
}

// Called by `vBlankTimer`
void ViewportRenderer::update() {
    juce::Time now = juce::Time::getCurrentTime();

    float delta = static_cast<float>((now - lastUpdateTime).inSeconds());
    smoothMouse = expEase(smoothMouse, mousePosition, 10.0, delta);
    repaint();
    lastUpdateTime = now;
}

void ViewportRenderer::render() {
    jassert(juce::OpenGLHelpers::isContextActive());

    auto desktopScale = (float) openGLContext.getRenderingScale();
    juce::OpenGLHelpers::clear(juce::Colour::fromFloatRGBA(0.05f, 0.05f, 0.05f, 1.0f));

    // Enable transparent rendering
    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE_MINUS_SRC_ALPHA);

    // Set render context bitmap size
    juce::gl::glViewport(
        0, 0,
        juce::roundToInt(desktopScale * (float) getWidth()),
        juce::roundToInt(desktopScale * (float) getHeight())
    );

    shaderProgram.use();
    opengl_assert();

    // Set projection matrix uniform if it has not been pruned
    if (projectionMatrixUniform.has_value()) {
        float halfWidth = 0.5f;
        float halfHeight = halfWidth * getLocalBounds().toFloat().getAspectRatio(false);

        auto projectionMatrix = juce::Matrix3D<float>::fromFrustum(
            -halfWidth, halfWidth,
            -halfHeight, halfHeight,
            1.0f, 10.0f
        ).mat;

        projectionMatrixUniform->setMatrix4(projectionMatrix, 1, false);
    }
    opengl_assert();

    // Set view matrix uniform if it has not been pruned
    if (viewMatrixUniform) {
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

        viewMatrixUniform->setMatrix4(((radius * pivot) * lift).mat, 1, false);
    }
    opengl_assert();

    // Draw vertex buffers
    for (BufferHandle const &bh : mesh.bufferHandles) {
        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, bh.vertexBuffer);
        juce::gl::glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, bh.indexBuffer);

        vertexAttributes.value().enable();

        juce::gl::glDrawElements(juce::gl::GL_TRIANGLES, bh.numIndices, juce::gl::GL_UNSIGNED_INT, nullptr);

        vertexAttributes.value().disable();
    }
    opengl_assert();

    // Reset the element buffers so child Components draw correctly
    juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
    opengl_assert();
}

void ViewportRenderer::paint(juce::Graphics &) {
    // Draw overtop of the OpenGL render
}

void ViewportRenderer::shutdown() {
    // May be called multiple times by the parent
    projectionMatrixUniform.reset();
    viewMatrixUniform.reset();
    shaderProgram.release();
    vertexAttributes.reset();
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
    viewport.setBounds(10, 10, 780, 450); // TODO make responsive layout
}
