#version 150

in float vScreenFacing;
in vec4 vColor;
in vec2 vTexCoord;
in vec3 vWorldPosition;
in vec3 vPosition;

uniform sampler2D texture0;
uniform float time;

out vec4 fragColor;


vec3 base_color() {
    const vec3 BASE_COLOR = vec3(0.2, 0.5, 0.35);
    const float LIFT = 0.01;

    float inv_facing = clamp(1.0 - vScreenFacing, 0.0, 1.0);
    float falloff = inv_facing * inv_facing + LIFT;
    return BASE_COLOR * falloff;
}


float softThreshold(float value, float threshold) {
    const float SMOOTH_STRENGTH = 1.5;
    const float SMOOTH_DILATE = 0.0;

    float width = fwidth(value) * SMOOTH_STRENGTH;
    float gradient = (threshold + (width * SMOOTH_DILATE) - value) / width;
    return clamp(gradient, 0.0, 1.0);
}

float wireframe_mask(float thickness) {
    float gradient = min(
        min(vTexCoord.x, vTexCoord.y), 
        1.0 - (vTexCoord.x + vTexCoord.y)
    );

    return softThreshold(gradient, thickness);
}

vec3 sparks() {
    const vec3 COLOR = vec3(1.0, 1.0, 2.0);
    const float BRIGHTNESS = 64.0;
    const float LIFT = 0.0025;
    const float THICKNESS = 1.0 / 32.0;

    const vec2 NOISE_SCALE = vec2(30.0, 64.0);
    const float NOISE_SPEED = 1.0;

    vec2 noise_coord = vPosition.xz / NOISE_SCALE + vec2(
        time * NOISE_SPEED / -129.0, // NPOT to maximize randomness
        time * NOISE_SPEED / -16.0
    );
    float noise = smoothstep(0.6, 0.9, texture(texture0, noise_coord).r);
    float intensity = pow(noise, 2.0) * BRIGHTNESS + LIFT;

    return COLOR * intensity * wireframe_mask(THICKNESS);
}


void main() {
    fragColor = vec4(
        base_color() + sparks(),
        vColor.a
    );
}   