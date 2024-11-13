varying vec3 worldPosition;
varying vec4 destinationColour;
varying vec2 textureCoordOut;

// Returns vec2(shadow, opacity) premultiplied
vec2 line(vec2 xy) {
    float width = 0.02;
    float fade = 0.01;

    float band = smoothstep(width + fade, width, abs(xy.x - 0.5));
    float shadow = 2.0 * abs(xy.y - 0.5);
    return vec2(sqrt(shadow) * band, band);
}

vec2 lines(vec2 xy) {
    return max(line(xy), line(xy.yx));
}

// Returns vec2(shadow, opacity) premultiplied
vec2 dots(vec2 xy) {
    float radius = 0.065;
    float fade = 0.01;

    float dist = length(xy - vec2(0.5));
    float value = smoothstep(radius + fade, radius, dist);
    return vec2(value, value);
}

float spotlight(vec2 uv) {
    float dist = 2.0 * length(uv - vec2(0.5));
    return smoothstep(1.0, 0.0, dist);
}

// Premultiplied alpha over, dimming the under's value
vec2 alphaOverDim(vec2 top, vec2 bot, float dim) {
    float inverse = 1.0 - top.y;
    float value = top.x + bot.x * inverse * dim;
    float alpha = top.y + bot.y * inverse;
    return vec2(value, alpha);
}

void main() {
    vec2 tiles    = fract(worldPosition.xy);
    vec2 subtiles = fract(worldPosition.xy * 4.0 + 0.5);

    vec2 big   = alphaOverDim(dots(tiles   ), lines(tiles   ), 0.5);
    vec2 small = alphaOverDim(dots(subtiles), lines(subtiles), 0.5);

    float texelDensity = length(dFdy(worldPosition.xy));
    float obliqueFade = pow(smoothstep(0.06, 0.0, texelDensity), 4);
    vec2 combined = alphaOverDim(big, small, obliqueFade * 0.5);

    float level = combined.x * combined.y * spotlight(textureCoordOut);

    gl_FragColor = vec4(destinationColour.rgb * level, destinationColour.a);
}