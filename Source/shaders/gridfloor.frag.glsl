varying vec3 worldPosition;
varying vec4 destinationColour;
varying vec2 textureCoordOut;

// Returns vec2(shadow, opacity) premultiplied
vec2 lines(vec2 coords) {
    float band = smoothstep(0.03, 0.02, abs(coords.x - 0.5));
    float shadow = 2.0 * abs(coords.y - 0.5);
    return vec2(sqrt(shadow) * band, band);
}

// Returns vec2(shadow, opacity) premultiplied
vec2 dots(vec2 coords) {
    float dist = length(coords - vec2(0.5));
    float value = smoothstep(0.075, 0.065, dist);
    return vec2(value, value);
}

float edgeFade(vec2 coords) {
    float dist = 2.0 * length(coords - vec2(0.5));
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
    vec2 pixelDensity = dFdy(worldPosition.xy);

    vec2 tiles = fract(worldPosition.xy);
    vec2 subtiles = fract(worldPosition.xy * 4.0 + 0.5);

    vec2 bigDots = dots(tiles);
    vec2 smallDots = dots(subtiles);

    vec2 bigLines = max(lines(tiles), lines(tiles.yx));
    vec2 smallLines = max(lines(subtiles), lines(subtiles.yx));

    vec2 big = alphaOverDim(bigDots, bigLines, 0.5);
    vec2 small = alphaOverDim(smallDots, smallLines, 0.5);

    float smudge = smoothstep(0.06, 0.0, length(pixelDensity));
    vec2 all = alphaOverDim(big, small, pow(smudge, 4) * 0.5);
    
    float edgeFade = edgeFade(textureCoordOut);

    float opacity = destinationColour.a * all.x * all.y * edgeFade;

    gl_FragColor = vec4(destinationColour.rgb, opacity);
}