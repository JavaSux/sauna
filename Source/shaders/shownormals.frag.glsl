#version 150

in vec3 vNormal;
in vec4 vColor;
in vec2 vTexCoord;

out vec4 fragColor;

void main() {
    fragColor = vec4(max(vNormal, 0), vColor.a);
}