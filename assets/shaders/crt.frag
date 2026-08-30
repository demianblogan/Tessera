uniform sampler2D texture;
uniform float time;

void main()
{
    vec2 textureCoordinates = gl_TexCoord[0].xy;

    // No screen curvature / barrel distortion: the picture stays flat and
    // nothing is cropped at the edges. Just the CRT "feel" below.

    // =====================================================
    // Chromatic aberration
    // =====================================================

    float chromaticAberrationStrength = 0.0005;

    float redChannel = texture2D(texture, textureCoordinates + vec2(chromaticAberrationStrength, 0.0)).r;
    float greenChannel = texture2D(texture, textureCoordinates).g;
    float blueChannel = texture2D(texture, textureCoordinates - vec2(chromaticAberrationStrength, 0.0)).b;

    vec3 finalColor = vec3(redChannel, greenChannel, blueChannel);

    // =====================================================
    // Scanlines  (slowly drifting, like a real CRT's vertical hold)
    // =====================================================

    float scanlineIntensity = sin((textureCoordinates.y * 1080.0 + time * 24.0) * 1.5) * 0.04;

    finalColor -= scanlineIntensity;

    // A faint mains-hum brightness flicker.
    finalColor *= 1.0 - 0.015 * sin(time * 8.0);

    // =====================================================
    // Vignette
    // =====================================================

    float distanceFromScreenCenter = distance(textureCoordinates, vec2(0.5));

    finalColor *= 1.0 - distanceFromScreenCenter * 0.5;

    gl_FragColor = vec4(finalColor, 1.0);
}
