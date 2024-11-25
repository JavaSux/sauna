#version 150

in vec2 vTexCoord;

uniform sampler2D sourceTexture;
uniform vec2 downsampleRatio;
uniform float strength;

out vec4 fragColor;

void main() {
    vec2 uv = vTexCoord / downsampleRatio;
    fragColor = texture(sourceTexture, uv, 0) * vec4(vec3(strength), 1.0);
}