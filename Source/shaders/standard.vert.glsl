#version 150

in vec3 aPosition;
in vec3 aNormal;
in vec4 aColor;
in vec2 aTexCoord;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

out vec4 vColor;
out vec2 vTexCoord;
out vec3 vWorldPosition;
// out vec3 vWorldNormal;
out float vScreenFacing;

void main() {
    vColor = aColor;
    vTexCoord = aTexCoord;
    vWorldPosition = (modelMatrix * vec4(aPosition, 1.0)).xyz;
    // mat3 worldNormalMatrix = mat3(transpose(inverse(modelMatrix)));
    // vWorldNormal = normalize(worldNormalMatrix * aNormal);

    mat4 modelViewMatrix = viewMatrix * modelMatrix;
    mat3 viewNormalMatrix = transpose(inverse(mat3(modelViewMatrix)));
    vec3 viewSpaceNormal = normalize(viewNormalMatrix * aNormal);
    vScreenFacing = dot(normalize(modelViewMatrix[3].xyz), -viewSpaceNormal);
    
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(aPosition, 1.0);
}
