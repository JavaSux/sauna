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