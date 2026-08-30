// Slow aurora curtains for the main-menu background. Drawn over a full-screen
// quad; no input texture. Deliberately dim -- it sits under everything else.

uniform float time;
uniform vec2 resolution;

// A couple of octaves of sine, cheap stand-in for noise.
float wave(float x, float t)
{
    return sin(x * 1.3 + t * 0.15) * 0.55
         + sin(x * 2.7 - t * 0.10) * 0.28
         + sin(x * 5.3 + t * 0.24) * 0.13;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution;

    vec3 colour = vec3(0.0);
    float amount = 0.0;

    for (int i = 0; i < 3; i++)
    {
        float fi = float(i);

        // Vertical centre of this curtain, gently waving.
        float centre = 0.34 + fi * 0.20 + wave(uv.x * 3.0 + fi * 7.0, time) * 0.05;
        float dist = abs(uv.y - centre);

        float band = smoothstep(0.16, 0.0, dist);
        band *= 0.55 + 0.45 * sin(uv.x * 9.0 + time * 0.5 + fi * 2.0);   // vertical streaks

        vec3 tint = mix(vec3(0.10, 0.85, 0.55), vec3(0.45, 0.28, 0.95), fi * 0.5);
        colour += tint * band;
        amount += band;
    }

    // Fade out at the very top and bottom edges.
    float vfade = smoothstep(0.0, 0.18, uv.y) * smoothstep(1.0, 0.62, uv.y);
    float alpha = clamp(amount, 0.0, 1.0) * 0.20 * vfade;

    gl_FragColor = vec4(colour, alpha);
}
