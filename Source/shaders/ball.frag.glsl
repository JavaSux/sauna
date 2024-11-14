varying vec3 worldPosition;
varying vec4 destinationColour;
varying vec2 textureCoordOut;

void main() {
    gl_FragColor = vec4(destinationColour.rgb, destinationColour.a);
}
