attribute vec3 position;
attribute vec3 normal;
attribute vec4 sourceColour;
attribute vec2 textureCoordIn;

void main() {
    gl_Position = vec4(position, 1.0);
}