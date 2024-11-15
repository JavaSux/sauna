varying vec3 worldPosition;
varying vec4 destinationColour;
varying vec2 textureCoordOut;

void main() {
    float dist = length(textureCoordOut - vec2(0.5));
    if (dist > 0.5) discard;

    vec2 xy = textureCoordOut * 2.0 - 1.0;
    float z = sqrt(1.0 - dot(xy, xy));

    gl_FragColor = vec4(vec3(xy, z) / 2.0 + 0.5, destinationColour.a);
}
