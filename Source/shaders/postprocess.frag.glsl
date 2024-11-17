uniform sampler2D renderedImage;
uniform int supersample;

void main() {
    ivec2 texCoord = ivec2(gl_FragCoord.xy) * supersample;
    vec4 color = vec4(0.0);
    
    for (int x = 0; x < supersample; ++x) {
        for (int y = 0; y < supersample; ++y) {
            color += texelFetch(renderedImage, texCoord + ivec2(x, y), 0);
        }
    }
    
    color /= float(supersample * supersample);
    gl_FragColor = color;
}