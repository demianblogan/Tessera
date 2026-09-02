// "Solidify" the paused game frame: quantise it into large square pixels and
// darken it a little, as if the picture froze and set. A moving front lets the
// effect sweep in and out instead of snapping.
//
//   reveal    fraction of the frame HEIGHT, measured from the top, that is
//             solidified. 0 = fully crisp, 1 = fully solidified.
//   frontSoft width in pixels of the soft band at the solidify front.
//   cellPx    size of a solidified pixel, in frame pixels.
//   darken    brightness multiplier applied where solidified (0..1).

uniform sampler2D texture;
uniform vec2 resolution;
uniform float cellPx;
uniform float darken;
uniform float reveal;
uniform float frontSoft;

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
    vec4 blocky = texture2D(texture, cellCentre / resolution);

    vec4 color = mix(crisp, blocky, solid);
    color.rgb *= mix(1.0, darken, solid);

    gl_FragColor = color * gl_Color;
}
