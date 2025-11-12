#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 viewSize;

// Luminance weights for grayscale conversion
const vec3 luminance = vec3(0.299, 0.587, 0.114);

void main()
{
    vec2 texelSize = 1.0 / viewSize;
    vec4 result = vec4(0.0);

    // 1. BLUR: 7x7 grid sampling (from -3 to +3)
    for (int x = -6; x <= 6; x++)
    {
        for (int y = -6; y <= 6; y++)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(texture0, fragTexCoord + offset) * fragColor;
        }
    }
    
    // Get the final averaged (blurred) color
    vec4 blurredColor = result / 169.0; // Use 49.0 for 7x7, or 81.0 for 9x9
    
    // 2. GRAYSCALE: Convert the blurred color to black and white
    float gray = dot(blurredColor.rgb, luminance);
    
    // 3. Set the final output
    finalColor = vec4(gray, gray, gray, blurredColor.a);
}