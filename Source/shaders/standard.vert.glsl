#version 150

in vec3 aPosition;
in vec3 aNormal;
in vec4 aColor;
in vec2 aTexCoord;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

out vec3 vWorldPosition;
out vec3 vNormal;
out vec4 vColor;
out vec2 vTexCoord;

void main() {
    vColor = aColor;
    vTexCoord = aTexCoord;
    vWorldPosition = aPosition;
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(aPosition, 1.0);
}
