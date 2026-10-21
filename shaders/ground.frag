#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;
in vec2 uv;
out vec4 fragColor;

uniform bool useObjectColor;
uniform vec3 objectColor;
uniform bool useTexture;
uniform sampler2D plazaTexture;
uniform vec2 textureRepeat;
uniform vec3 lightDirection;
uniform float ambientStrength;
uniform float diffuseStrength;

void main()
{
    vec3 baseColor = objectColor;

    if (!useObjectColor)
    {
        vec2 cell = floor(worldPosition.xz);
        float checker = mod(cell.x + cell.y, 2.0);
        baseColor = mix(vec3(0.48, 0.56, 0.42), vec3(0.51, 0.59, 0.45), checker);
    }

    if (useTexture)
    {
        baseColor *= texture(plazaTexture, uv * textureRepeat).rgb;
    }

    // lightDirection describes the direction sunlight travels in world space.
    vec3 normal = normalize(worldNormal);
    vec3 toLight = normalize(-lightDirection);
    float lambert = max(dot(normal, toLight), 0.0);
    vec3 color = baseColor * (ambientStrength + diffuseStrength * lambert);

    if (!useObjectColor)
    {
        float fade = smoothstep(200.0, 400.0, length(worldPosition.xz));
        color = mix(color, vec3(0.65, 0.80, 0.92), fade);
    }

    fragColor = vec4(color, 1.0);
}
