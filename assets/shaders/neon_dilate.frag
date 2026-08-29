uniform sampler2D texture;

// Per-sample texel step along one axis: (step, 0) for the horizontal pass,
// (0, step) for the vertical one. Running both gives a square dilation that
// grows the silhouette evenly on every side and corner.
uniform vec2 offset;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;

    float a = texture2D(texture, uv).a;

    a = max(a, texture2D(texture, uv + offset * 1.0).a);
    a = max(a, texture2D(texture, uv - offset * 1.0).a);
    a = max(a, texture2D(texture, uv + offset * 2.0).a);
    a = max(a, texture2D(texture, uv - offset * 2.0).a);
    a = max(a, texture2D(texture, uv + offset * 3.0).a);
    a = max(a, texture2D(texture, uv - offset * 3.0).a);
    a = max(a, texture2D(texture, uv + offset * 4.0).a);
    a = max(a, texture2D(texture, uv - offset * 4.0).a);

    gl_FragColor = gl_Color * vec4(1.0, 1.0, 1.0, a);
}
