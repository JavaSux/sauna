attribute vec3 position;
attribute vec3 normal;
attribute vec4 sourceColour;
attribute vec2 textureCoordIn;

varying vec2 textureCoordOut;

void main() {
    textureCoordOut = textureCoordIn;
    gl_Position = vec4(position, 1.0);
}