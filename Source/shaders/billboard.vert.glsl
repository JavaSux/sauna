attribute vec3 position;
attribute vec3 normal;
attribute vec4 sourceColour;
attribute vec2 textureCoordIn;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

varying vec4 destinationColour;
varying vec2 textureCoordOut;
varying vec3 worldPosition;

void main() {
    destinationColour = sourceColour;
    textureCoordOut = textureCoordIn;
    worldPosition = position;

    mat4 modelView = viewMatrix * modelMatrix;

    // Reset rotation mat3 to identity
    mat4 billboard = mat4(
        vec4(1.0, 0.0, 0.0, modelView[0][3]),
        vec4(0.0, 1.0, 0.0, modelView[1][3]),
        vec4(0.0, 0.0, 1.0, modelView[2][3]),
        modelView[3]
    );

    gl_Position = projectionMatrix * billboard * vec4(position, 1.0);
}