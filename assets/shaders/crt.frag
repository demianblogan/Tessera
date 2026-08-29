uniform sampler2D texture;
uniform float time;

void main()
{
    vec2 textureCoordinates = gl_TexCoord[0].xy;

    // =====================================================
    // CRT curvature
    // =====================================================
    
    float screenDistortionStrength = 0.03;

    vec2 curvedTextureCoordinates = textureCoordinates * 2.0 - 1.0;
    curvedTextureCoordinates += curvedTextureCoordinates * length(curvedTextureCoordinates) * screenDistortionStrength;
    curvedTextureCoordinates = (curvedTextureCoordinates + 1.0) * 0.5;

    // =====================================================
    // Outside screen
    // =====================================================

    if (curvedTextureCoordinates.x < 0.0 ||
        curvedTextureCoordinates.x > 1.0 ||
        curvedTextureCoordinates.y < 0.0 ||
        curvedTextureCoordinates.y > 1.0)
    {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // =====================================================
    // Chromatic aberration
    // =====================================================

    float chromaticAberrationStrength = 0.0005;

    float redChannel = texture2D(texture, curvedTextureCoordinates + vec2(chromaticAberrationStrength, 0.0)).r;
    float greenChannel = texture2D(texture, curvedTextureCoordinates).g;
    float blueChannel = texture2D(texture, curvedTextureCoordinates - vec2(chromaticAberrationStrength, 0.0)).b;

    vec3 finalColor = vec3(redChannel, greenChannel, blueChannel);

    // =====================================================
    // Scanlines  (slowly drifting, like a real CRT's vertical hold)
    // =====================================================

    float scanlineIntensity = sin((curvedTextureCoordinates.y * 1080.0 + time * 24.0) * 1.5) * 0.04;

    finalColor -= scanlineIntensity;

    // A faint mains-hum brightness flicker.
    finalColor *= 1.0 - 0.015 * sin(time * 8.0);

    // =====================================================
    // Vignette
    // =====================================================

    float distanceFromScreenCenter = distance(curvedTextureCoordinates, vec2(0.5));

    finalColor *= 1.0 - distanceFromScreenCenter * 0.5;

    gl_FragColor = vec4(finalColor, 1.0);
}