varying vec2 textureCoordOut;

uniform sampler2D sourceTexture;
uniform vec2 sourceResolution;
uniform bool vertical;

// https://lisyarus.github.io/blog/posts/blur-coefficients-generator.html
// Disabled linear filtering, disable SSC, radius 7
const int SIZE = 15;
const float WEIGHTS[SIZE] = float[](
    0.0008127702630692522,
    0.0034457364521441264,
    0.011697321430725573,
    0.0317966162867859,
    0.06920946673711453,
    0.1206258023667399,
    0.16834686856900122,
    0.18813083578883916,
    0.16834686856900122,
    0.1206258023667399,
    0.06920946673711453,
    0.0317966162867859,
    0.011697321430725573,
    0.0034457364521441264,
    0.0008127702630692522
);

vec4 blur(vec2 blurDirection) {
    vec4 sum = vec4(0.0);

    vec2 fullResolution = vec2(textureSize(sourceTexture, 0));
    vec2 scale = sourceResolution / fullResolution;
    for (int i = 0; i < SIZE; ++i) {
        vec2 offset = blurDirection * float(i - 7) / sourceResolution;
        sum += texture(sourceTexture, clamp(textureCoordOut + offset, 0, 1) * scale) * WEIGHTS[i];
    }

    return sum;
}

void main() {
    vec2 direction = vertical ? vec2(0.0, 1.0) : vec2(1.0, 0.0);
	gl_FragColor = blur(direction);
}