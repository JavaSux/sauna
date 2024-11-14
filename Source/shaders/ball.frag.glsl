varying vec3 worldPosition;
varying vec4 destinationColour;
varying vec2 textureCoordOut;

void main() {
    gl_FragColor = vec4(textureCoordOut.rg, 0.5, destinationColour.a);
}
