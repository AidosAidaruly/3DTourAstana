#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;
in vec2 uv;
in vec3 localPosition;
in vec3 worldViewDirection;
out vec4 fragColor;

uniform bool useObjectColor;
uniform vec3 objectColor;
uniform bool useTexture;
uniform sampler2D plazaTexture;
uniform vec2 textureRepeat;
uniform vec3 lightDirection;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform bool goldenPanels;
uniform bool shaftGlass;
uniform bool useGrass;

vec3 shadeGoldenPanels(vec3 normal, vec3 toLight, float lambert)
{
    const float pi = 3.14159265359;
    const vec2 panelCount = vec2(32.0, 14.0);
    vec3 sphereDirection = normalize(localPosition);
    float longitude = atan(sphereDirection.z, sphereDirection.x) / (2.0 * pi) + 0.5;
    float latitude = acos(clamp(sphereDirection.y, -1.0, 1.0)) / pi;
    float row = floor(latitude * panelCount.y);
    vec2 grid = vec2(longitude * panelCount.x + mod(row, 2.0) * 0.5,
                     latitude * panelCount.y);

    // Smooth narrow, staggered glass joints without aliasing at a distance.
    float horizontalRadius = max(length(sphereDirection.xz), 0.05);
    vec2 smoothing = clamp(vec2(
        length(fwidth(sphereDirection.xz)) * panelCount.x / (2.0 * pi * horizontalRadius),
        length(fwidth(sphereDirection)) * panelCount.y / pi
    ) * 0.6, vec2(0.002), vec2(0.25));
    vec2 edgeDistance = abs(fract(grid + 0.5) - 0.5);
    vec2 joints = 1.0 - smoothstep(
        vec2(0.012) - smoothing, vec2(0.012) + smoothing, edgeDistance
    );
    joints.x *= smoothstep(0.04, 0.15, length(sphereDirection.xz));
    float joint = max(joints.x, joints.y);

    // Neighboring panes vary gently between champagne and amber.
    vec2 panel = floor(grid);
    float variation = 0.5 + 0.5 * sin(panel.x * 1.73 + panel.y * 2.19);
    vec3 gold = mix(vec3(0.87, 0.66, 0.30), vec3(0.92, 0.73, 0.37), variation);
    vec3 color = gold * (0.76 + 0.24 * lambert);

    vec3 viewDirection = normalize(worldViewDirection);
    vec3 halfDirection = normalize(toLight + viewDirection);
    float highlight = pow(max(dot(normal, halfDirection), 0.0), 160.0);
    float broadHighlight = pow(max(dot(normal, halfDirection), 0.0), 28.0);
    float fresnel = pow(1.0 - max(dot(normal, viewDirection), 0.0), 5.0);

    // Analytic environment reflection: cool sky above a warm, hazy horizon.
    // This stays stable as the camera moves and needs no cubemap or new assets.
    vec3 reflection = reflect(-viewDirection, normal);
    float sky = smoothstep(-0.12, 0.42, reflection.y);
    vec3 environment = mix(vec3(0.86, 0.71, 0.48), vec3(0.48, 0.72, 0.91), sky);
    float horizonGlow = 1.0 - smoothstep(0.02, 0.28, abs(reflection.y));
    environment += vec3(0.15, 0.12, 0.08) * horizonGlow;

    color = mix(color, environment, 0.36 + 0.40 * fresnel);
    color += vec3(1.0, 0.91, 0.69) * (0.70 * highlight + 0.18 * broadHighlight);

    // Fine champagne mullions sit on the glass without thick black outlines.
    return mix(color, color * vec3(0.86, 0.84, 0.80), joint * 0.48);
}

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

    if (useGrass)
    {
        // Soft world-space variation breaks up the flat color without harsh noise.
        float broad = sin(worldPosition.x * 0.83) * sin(worldPosition.z * 0.76);
        float fine = sin(worldPosition.x * 2.4 + worldPosition.z * 1.9);
        baseColor *= 0.98 + 0.03 * broad + 0.012 * fine;
    }

    // lightDirection describes the direction sunlight travels in world space.
    vec3 normal = normalize(worldNormal);
    vec3 toLight = normalize(-lightDirection);
    float lambert = max(dot(normal, toLight), 0.0);
    vec3 color = baseColor * (ambientStrength + diffuseStrength * lambert);

    if (goldenPanels)
    {
        color = shadeGoldenPanels(normal, toLight, lambert);
    }

    if (shaftGlass)
    {
        vec3 viewDirection = normalize(worldViewDirection);
        vec3 reflection = reflect(-viewDirection, normal);
        float fresnel = pow(1.0 - max(dot(normal, viewDirection), 0.0), 5.0);
        vec3 reflectedSky = mix(
            vec3(0.55, 0.67, 0.72), vec3(0.65, 0.81, 0.93),
            smoothstep(-0.2, 0.5, reflection.y)
        );
        vec3 halfDirection = normalize(toLight + viewDirection);
        float highlight = pow(max(dot(normal, halfDirection), 0.0), 96.0);
        color = mix(color, reflectedSky, 0.22 + 0.28 * fresnel);
        color += vec3(0.70, 0.83, 0.91) * highlight * 0.35;
    }

    if (!useObjectColor)
    {
        float fade = smoothstep(200.0, 400.0, length(worldPosition.xz));
        color = mix(color, vec3(0.65, 0.80, 0.92), fade);
    }

    fragColor = vec4(color, 1.0);
}
