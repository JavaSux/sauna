#include "Viewport.h"

const juce::Point<float> ViewportComponent::INITIAL_MOUSE{ 0.5f, 0.6f };
const juce::Colour ViewportComponent::CLEAR_COLOR = juce::Colours::black;
const double ViewportComponent::MOUSE_DELAY = 0.4;

void ViewportComponent::initialise() {
    // May be called mulitple times by the parent
    DBG("Initializing ViewportComponent resources");

    juce::String
        standardVS       { juce::OpenGLHelpers::translateVertexShaderToV3(BinaryData::standard_vert_glsl) },
        billboardVS      { juce::OpenGLHelpers::translateVertexShaderToV3(BinaryData::billboard_vert_glsl) },
        postprocessVS    { juce::OpenGLHelpers::translateVertexShaderToV3(BinaryData::postprocess_vert_glsl) },
        gridFloorFS      { juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::gridfloor_frag_glsl) },
        ballFS           { juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::ball_frag_glsl) },
        downsampleFS     { juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::downsample_frag_glsl) },
        cinematicFS      { juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::cinematic_frag_glsl) },
		gaussianFS       { juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::gaussian_frag_glsl) },
		bloomAccumulateFS{ juce::OpenGLHelpers::translateFragmentShaderToV3(BinaryData::bloomAccumulate_frag_glsl) };

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
    cinematicShader  = loadShader(openGLContext, postprocessVS, cinematicFS, "cinematicShader");
	gaussianShader = loadShader(openGLContext, postprocessVS, gaussianFS, "gaussianShader");
	bloomAccumulateShader = loadShader(openGLContext, postprocessVS, bloomAccumulateFS, "bloomAccumulateShader");
    postprocess.emplace(
        downsampleShader,
        cinematicShader,
        gaussianShader,
		bloomAccumulateShader,
        juce::Point<int>{ componentBounds.getWidth(), componentBounds.getHeight() },
        SUPERSAMPLE
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
    using namespace juce::gl;

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

        glBindBuffer(GL_ARRAY_BUFFER, mesh.bufferHandle.vertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.bufferHandle.indexBuffer);

        mesh.attribs.enable();
        mesh.bufferHandle.drawElements();
        mesh.attribs.disable();
    } };

    postprocess->sizeTo({ componentBounds.getWidth(), componentBounds.getHeight() }, SUPERSAMPLE);

    /* ===================================== */
    /* Scene rendering */

    postprocess->rasterBuffer.setRenderTarget();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    // Clear frame, depth, stencil. Must happen after re-enabling depth mask
    juce::OpenGLHelpers::clear(CLEAR_COLOR);

    draw(ball.value());

    // Additive rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);

    // Read-only depth buffer for transparent elements
    glDepthMask(GL_FALSE);

    draw(gridFloor.value());

    // Apply postprocessing
	postprocess->process(0); // 0 is the presentation buffer

    // Reset the element buffers so child Components draw correctly
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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