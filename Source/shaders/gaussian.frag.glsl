#version 150

in vec2 vTexCoord;

uniform sampler2D sourceTexture;
uniform vec2 sourceResolution;
uniform bool vertical;

out vec4 fragColor;

// https://lisyarus.github.io/blog/posts/blur-coefficients-generator.html
// Radius 7
const int SAMPLE_COUNT = 8;

const float OFFSETS[8] = float[8](
    -6.328357272092126,
    -4.378621204796657,
    -2.431625915613778,
    -0.4862426846689484,
    1.4588111840004858,
    3.4048471718931532,
    5.353083811756559,
    7
);

const float WEIGHTS[8] = float[8](
    0.027508406306604068,
    0.08940648616079577,
    0.18921490087565024,
    0.26088633929947086,
    0.2343989200518563,
    0.13722534949218246,
    0.052327012559001844,
    0.009032585254438357
);

vec4 blur(vec2 blurDirection) {
    vec4 sum = vec4(0.0);

    vec2 fullResolution = vec2(textureSize(sourceTexture, 0));
    vec2 scale = sourceResolution / fullResolution;
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        vec2 offset = blurDirection * OFFSETS[i] / sourceResolution;
        sum += texture(sourceTexture, clamp(vTexCoord + offset, 0, 1) * scale) * WEIGHTS[i];
    }

    return sum;
}

void main() {
    vec2 direction = vertical ? vec2(0.0, 1.0) : vec2(1.0, 0.0);
	fragColor = blur(direction);
}