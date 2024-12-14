#version 150

in float vScreenFacing;
in vec4 vColor;
in vec2 vTexCoord;

out vec4 fragColor;

const float SMOOTH_STRENGTH = 1.5;

const float BASE_BRIGHTNESS = 0.5;
const float WIREFRAME_BRIGHTNESS = 3;

const vec3 BASE_COLOR = vec3(0.4, 1.0, 0.7);
const vec3 WIREFRAME_COLOR = vec3(1);

float pixelPitch(float axis) {
    // return length(vec2(dFdx(axis), dFdy(axis)));
    return fwidth(axis);
}

float softThreshold(float value, float threshold) {
    float width = pixelPitch(value) * SMOOTH_STRENGTH;
    float gradient = (threshold + (width / 2.0) - value) / width;
    return clamp(gradient, 0.0, 1.0);
}

float wireframe(float thickness) {
    float gradient = min(
        min(vTexCoord.x, vTexCoord.y), 
        1.0 - (vTexCoord.x + vTexCoord.y)
    );
    float d = softThreshold(gradient, thickness);

    return d;
}

float oblique() {
    float inv_facing = 1.0 - vScreenFacing;
    return inv_facing * inv_facing;
}

void main() {
    float falloff = oblique();

    vec3 base_color = BASE_COLOR * BASE_BRIGHTNESS * falloff;
    vec3 wireframe_color = WIREFRAME_COLOR 
        * WIREFRAME_BRIGHTNESS 
        * wireframe(1.0 / 48.0) 
        * (1.0 - falloff);

    fragColor = vec4(
        base_color + wireframe_color, 
        vColor.a
    );
}   