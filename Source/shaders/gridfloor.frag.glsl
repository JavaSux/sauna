#version 150

in vec3 vWorldPosition;
in vec4 vColor;
in vec2 vTexCoord;
in float vScreenFacing;

out vec4 fragColor;

const float BRIGHTNESS = 1.5;

const float SMOOTH_STRENGTH = 1.5;
const float SMOOTH_DILATE = 0.5;

const float LINE_WIDTH = 0.03;
const float DOT_RADIUS = 0.065;

float square(float value) {
    return value * value;
}

float pixelPitch(float axis) {
    // return length(vec2(dFdx(axis), dFdy(axis)));
    return fwidth(axis);
}

float softThreshold(float value, float threshold) {
    float width = pixelPitch(value) * SMOOTH_STRENGTH;
    float gradient = (threshold + (width * SMOOTH_DILATE) - value) / width;
    return clamp(gradient, 0.0, 1.0);
}

// Returns vec2(shadow, opacity) premultiplied
vec2 line(vec2 xy) {
    vec2 dxy = abs(xy - 0.5);
    float band = softThreshold(dxy.x, LINE_WIDTH);
    float shadow = 2.0 * dxy.y;

    return vec2(sqrt(shadow) * band, band);
}

vec2 lines(vec2 xy) {
    return max(line(xy), line(xy.yx));
}

// Returns vec2(shadow, opacity) premultiplied
vec2 dots(vec2 xy) {
    float dist = length(xy - vec2(0.5));
    float value = softThreshold(dist, DOT_RADIUS);

    return vec2(value, value);
}

float spotlight(vec2 uv) {
    float dist = 2.0 * length(uv - vec2(0.5));
    return smoothstep(1.0, 0.0, dist);
}

// Premultiplied alpha over, dimming `bottom`'s value by `dim`
vec2 alphaOverDim(vec2 top, vec2 bottom, float dim) {
    float inverse = 1.0 - top.y;
    float value = top.x + bottom.x * inverse * dim;
    float alpha = top.y + bottom.y * inverse;
    return vec2(value, alpha);
}

void main() {
    vec2 tiles    = fract(vWorldPosition.xy);
    vec2 subtiles = fract(vWorldPosition.xy * 4.0 + 0.5);

    vec2 big   = alphaOverDim(dots(tiles   ), lines(tiles   ), 0.5);
    vec2 small = alphaOverDim(dots(subtiles), lines(subtiles), 0.5);

    float texelDensity = length(dFdy(vWorldPosition.xy));
    float facing = abs(vScreenFacing);
    float minorFade = clamp(facing * 2.0, 0.0, 1.0);
    float majorFade = clamp(facing * 4.0, 0.0, 1.0);

    vec2 combined = alphaOverDim(big, small, minorFade * 0.5);
    float value = combined.x * combined.y * spotlight(vTexCoord) * majorFade;
    
    fragColor = vec4(vColor.rgb * value * BRIGHTNESS, 1.0);
}