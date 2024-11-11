#include "SaunaProcessor.h"
#include "SaunaEditor.h"

#include <string_view>
#include <stdint.h>

static char const *const VERTEX_SHADER = R"(
attribute vec3 position;
attribute vec3 normal;
attribute vec4 sourceColour;
attribute vec2 textureCoordIn;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

varying vec4 destinationColour;
varying vec2 textureCoordOut;
varying vec3 worldPosition;

void main() {
    destinationColour = sourceColour;
    textureCoordOut = textureCoordIn;
    worldPosition = position;
    gl_Position = projectionMatrix * viewMatrix * vec4(position, 1.0);
}
)";

static char const *const FRAGMENT_SHADER = R"(
varying vec3 worldPosition;
varying vec4 destinationColour;
varying vec2 textureCoordOut;

// Returns vec2(shadow, opacity) premultiplied
vec2 lines(vec2 coords) {
    float band = smoothstep(0.03, 0.02, abs(coords.x - 0.5));
    float shadow = 2.0 * abs(coords.y - 0.5);
    return vec2(sqrt(shadow) * band, band);
}

// Returns vec2(shadow, opacity) premultiplied
vec2 dots(vec2 coords) {
    float dist = length(coords - vec2(0.5));
    float value = smoothstep(0.075, 0.065, dist);
    return vec2(value, value);
}

float edgeFade(vec2 coords) {
    float dist = 2.0 * length(coords - vec2(0.5));
    return smoothstep(1.0, 0.0, dist);
}

// Premultiplied alpha over, dimming the under's value
vec2 alphaOverDim(vec2 top, vec2 bot, float dim) {
    float inverse = 1.0 - top.y;
    float value = top.x + bot.x * inverse * dim;
    float alpha = top.y + bot.y * inverse;
    return vec2(value, alpha);
}

void main() {
    vec2 pixelDensity = dFdy(worldPosition.xy);

    vec2 tiles = fract(worldPosition.xy);
    vec2 subtiles = fract(worldPosition.xy * 4.0 + 0.5);

    vec2 bigDots = dots(tiles);
    vec2 smallDots = dots(subtiles);

    vec2 bigLines = max(lines(tiles), lines(tiles.yx));
    vec2 smallLines = max(lines(subtiles), lines(subtiles.yx));

    vec2 big = alphaOverDim(bigDots, bigLines, 0.5);
    vec2 small = alphaOverDim(smallDots, smallLines, 0.5);

    float smudge = smoothstep(0.06, 0.0, length(pixelDensity));
    vec2 all = alphaOverDim(big, small, pow(smudge, 4) * 0.5);
    
    float edgeFade = edgeFade(textureCoordOut);

    float opacity = destinationColour.a * all.x * all.y * edgeFade;

    gl_FragColor = vec4(destinationColour.rgb, opacity);
}
)";

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
    if (position.has_value()) {
        juce::gl::glVertexAttribPointer(
            position->attributeID,
            3,
            juce::gl::GL_FLOAT,
            juce::gl::GL_FALSE,
            sizeof (Vertex),
            nullptr
        );
        juce::gl::glEnableVertexAttribArray(position->attributeID);
    }

    if (normal.has_value()) {
        juce::gl::glVertexAttribPointer(
            normal->attributeID, 
            3, 
            juce::gl::GL_FLOAT, 
            juce::gl::GL_FALSE, 
            sizeof (Vertex), 
            (GLvoid*) (sizeof (float) * 3)
        );
        juce::gl::glEnableVertexAttribArray(normal->attributeID);
    }

    if (sourceColour.has_value()) {
        juce::gl::glVertexAttribPointer(
            sourceColour->attributeID, 
            4, 
            juce::gl::GL_FLOAT, 
            juce::gl::GL_FALSE, 
            sizeof (Vertex), 
            (GLvoid*) (sizeof (float) * 6)
        );
        juce::gl::glEnableVertexAttribArray(sourceColour->attributeID);
    }

    if (textureCoordIn.has_value()) {
        juce::gl::glVertexAttribPointer(
            textureCoordIn->attributeID, 
            2, 
            juce::gl::GL_FLOAT, 
            juce::gl::GL_FALSE, 
            sizeof (Vertex), 
            (GLvoid*) (sizeof (float) * 10)
        );
        juce::gl::glEnableVertexAttribArray(textureCoordIn->attributeID);
    }
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


ViewportRenderer::ViewportRenderer() :
    juce::OpenGLAppComponent{},
    shaderProgram{openGLContext}
{
    setOpaque(true);
}

ViewportRenderer::~ViewportRenderer() {
    shutdownOpenGL();
}

void ViewportRenderer::initialise() {
    // May be called mulitple times by the parent
    DBG("Initializing shaders");
    shaderProgram.release();

    if (
        !shaderProgram.addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(VERTEX_SHADER))
        || !shaderProgram.addFragmentShader(juce::OpenGLHelpers::translateFragmentShaderToV3(FRAGMENT_SHADER))
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
        juce::Matrix3D<float> position = juce::Matrix3D<float>::fromTranslation({ 0.0f, 5.0f, -2.0f });
        juce::Matrix3D<float> rotation = position.rotation({ -1.2f , 0.0f, 0.0f });

        viewMatrixUniform->setMatrix4((rotation * position).mat, 1, false);
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
