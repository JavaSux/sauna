varying vec2 textureCoordOut;

uniform sampler2D sourceTexture;
uniform vec2 downsampleRatio;
uniform float strength;

void main() {
    vec2 uv = textureCoordOut / downsampleRatio;
    gl_FragColor = texture(sourceTexture, uv, 0) * vec4(vec3(strength), 1.0);
}