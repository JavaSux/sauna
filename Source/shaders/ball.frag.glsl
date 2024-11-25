#version 150

in vec4 vColor;
in vec2 vTexCoord;

out vec4 fragColor;

const float BRIGHTNESS = 5.0;

void main() {
    float dist = length(vTexCoord - vec2(0.5));
    if (dist > 0.5) discard;

    vec2 xy = vTexCoord * 2.0 - 1.0;
    float z = sqrt(1.0 - max(0.9999, dot(xy, xy)));

    fragColor = vec4(
        (vec3(xy, z) / 2.0 + 0.5) * BRIGHTNESS, 
        vColor.a
    );
}