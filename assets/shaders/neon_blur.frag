uniform sampler2D texture;

// Texel step for this pass, e.g. (radius / width, 0) for a horizontal pass and
// (0, radius / height) for a vertical one. Run the pair to get a full 2D blur;
// run the pair repeatedly to widen it.
uniform vec2 direction;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;

    // Nine-tap Gaussian collapsed to five weighted samples.
    vec4 color = texture2D(texture, uv) * 0.2270270270;
    color += texture2D(texture, uv + direction * 1.3846153846) * 0.3162162162;
    color += texture2D(texture, uv - direction * 1.3846153846) * 0.3162162162;
    color += texture2D(texture, uv + direction * 3.2307692308) * 0.0702702703;
    color += texture2D(texture, uv - direction * 3.2307692308) * 0.0702702703;

    gl_FragColor = gl_Color * color;
}
