uniform sampler2D downsampledImage;

varying vec2 textureCoordOut;

const float EXPOSURE = 10.0;
const vec3 LIFT = vec3(0.02);
const float WHITE_LEVEL = 11.2;
const float ABBERATION = 1.0 / 256.0;
const float VIGNETTE_STRENGTH = 0.5;

// ==== UV operations ====
float aspect_ratio(sampler2D image) {
    vec2 resolution = vec2(textureSize(image, 0));
    return resolution.x / resolution.y;
}

vec2 center(vec2 uv, float aspect_ratio) {
    return (uv * 2.0 - 1.0) / vec2(1.0, aspect_ratio);
}

vec2 uncenter(vec2 uv, float aspect_ratio) {
    return uv * vec2(1.0, aspect_ratio) * 0.5 + 0.5;
}


// ==== Tonemapping ====
vec3 uncharted2(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 white = uncharted2(vec3(WHITE_LEVEL));
vec3 tonemap(vec3 color) {
    return uncharted2(color * EXPOSURE) / white + LIFT;
}


// ==== Lens effects ====
float vignette(vec2 centered_uv) {
    float offset = 1.4;
    float fade = 0.7;
    return 1.0 - smoothstep(offset - fade, offset, length(centered_uv)) * VIGNETTE_STRENGTH;
}

vec3 sample_abberated(vec2 centered_uv, float aspect_ratio) {
    vec3 colors[5];
    for (int i=0; i<5; ++i) {
        vec2 uv = uncenter(centered_uv * (1.0 - float(i) * ABBERATION), aspect_ratio);
        colors[i] = texture(downsampledImage, uv).rgb;
    }

    return vec3(
        (colors[4].r + 2.0 * colors[3].r + colors[2].r) / 4.0,
        (colors[3].g + 2.0 * colors[2].g + colors[1].g) / 4.0,
        (colors[2].b + 2.0 * colors[1].b + colors[0].b) / 4.0
    );
}


void main() {
    float aspect_ratio = aspect_ratio(downsampledImage);
    vec2 centered_uv = center(textureCoordOut, aspect_ratio);
    vec3 color = sample_abberated(centered_uv, aspect_ratio);

    gl_FragColor = vec4(tonemap(color * vignette(centered_uv)), 1.0);
}