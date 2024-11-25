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

    // Reset rotation mat3 to identity
    mat4 billboard = mat4(
        vec4(1.0, 0.0, 0.0, modelView[0][3]),
        vec4(0.0, 1.0, 0.0, modelView[1][3]),
        vec4(0.0, 0.0, 1.0, modelView[2][3]),
        modelView[3]
    );

    gl_Position = projectionMatrix * billboard * vec4(aPosition, 1.0);
}