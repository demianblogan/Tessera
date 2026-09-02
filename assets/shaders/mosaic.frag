// "Solidify" the paused game frame: blur it, quantise it into large square
// pixels and darken it, as if the picture froze and set. A moving front lets
// the effect sweep in and out instead of snapping.
//
//   reveal    fraction of the frame HEIGHT, measured from the top, that is
//             solidified. 0 = fully crisp, 1 = fully solidified.
//   frontSoft width in pixels of the soft band at the solidify front.
//   cellPx    size of a solidified pixel, in frame pixels.
//   blurPx    blur step (roughly the blur radius), in frame pixels.
//   darken    brightness multiplier applied where solidified (0..1).

uniform sampler2D texture;
uniform vec2 resolution;
uniform float cellPx;
uniform float blurPx;
uniform float darken;
uniform float reveal;
uniform float frontSoft;

// 3x3 tent blur at a step of blurPx.
vec4 blurred(vec2 uv)
{
    vec2 s = blurPx / resolution;
    vec4 c = texture2D(texture, uv) * 4.0;
    c += texture2D(texture, uv + vec2( s.x, 0.0)) * 2.0;
    c += texture2D(texture, uv + vec2(-s.x, 0.0)) * 2.0;
    c += texture2D(texture, uv + vec2( 0.0,  s.y)) * 2.0;
    c += texture2D(texture, uv + vec2( 0.0, -s.y)) * 2.0;
    c += texture2D(texture, uv + vec2( s.x,  s.y));
    c += texture2D(texture, uv + vec2( s.x, -s.y));
    c += texture2D(texture, uv + vec2(-s.x,  s.y));
    c += texture2D(texture, uv + vec2(-s.x, -s.y));
    return c / 16.0;
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec2 px = uv * resolution;

    // The frame buffer is stored bottom-up, so measure from the bottom edge to
    // get a visually top-down sweep.
    float y = resolution.y - px.y;
    float frontY = reveal * resolution.y;
    float solid = 1.0 - smoothstep(frontY - frontSoft, frontY + frontSoft, y);

    vec2 cellCentre = floor(px / cellPx) * cellPx + cellPx * 0.5;
    vec4 crisp = texture2D(texture, uv);
    vec4 blocky = blurred(cellCentre / resolution);

    vec4 color = mix(crisp, blocky, solid);
    color.rgb *= mix(1.0, darken, solid);

    gl_FragColor = color * gl_Color;
}
