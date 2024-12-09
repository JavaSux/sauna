#version 150

in vec3 aPosition;
in vec4 aColor;
in vec2 aTexCoord;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

out vec4 vColor;
out vec2 vTexCoord;
out vec3 vWorldPosition;

void main() {
    vColor = aColor;
    vTexCoord = aTexCoord;
    vWorldPosition = aPosition;

    mat4 modelView = viewMatrix * modelMatrix;

    // Maybe wrong, not sure how to handle W component
    mat4 billboard = mat4(
        modelMatrix[0],
        modelMatrix[1],
        modelMatrix[3],
        modelView[3]
    );

    gl_Position = projectionMatrix * billboard * vec4(aPosition, 1.0);
}