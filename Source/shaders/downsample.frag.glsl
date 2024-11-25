#version 150

uniform sampler2D renderedImage;
uniform int supersample;

out vec4 fragColor;

void main() {
    ivec2 texCoord = ivec2(gl_FragCoord.xy) * supersample;
    vec3 color = vec3(0.0);
    
    for (int x = 0; x < supersample; ++x) {
        for (int y = 0; y < supersample; ++y) {
            color += texelFetch(renderedImage, texCoord + ivec2(x, y), 0).rgb;
        }
    }
    
    color /= float(supersample * supersample);

    fragColor = vec4(color, 1.0);
}