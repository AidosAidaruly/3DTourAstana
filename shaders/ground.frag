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
uniform bool nurAlemFacade;
uniform bool abuDhabiFacade;
uniform int abuDhabiSecondaryFacadeMode;
uniform bool abuDhabiGlassMaterial;
uniform bool useGrass;
uniform bool useAsphalt;
uniform bool akOrdaStone;
uniform bool akOrdaRoof;
uniform bool akOrdaWater;
uniform bool akOrdaWaterJet;
uniform vec2 akOrdaWaterCenter;
uniform bool botanicalLake;
uniform bool greenhouseGlass;
uniform bool expoCanopyGlass;
uniform float sceneTime;
uniform int talanFacadeMode;

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

vec3 shadeAbuDhabiGlass(vec3 baseColor, vec3 normal, vec3 toLight, float lambert)
{
    vec3 viewDirection = normalize(worldViewDirection);
    vec3 reflection = reflect(-viewDirection, normal);
    float facing = clamp(dot(normal, viewDirection), 0.0, 1.0);
    float fresnel = 0.04 + 0.96 * pow(1.0 - facing, 5.0);

    // A soft directional sky reflection adds depth without a cubemap.
    float skyHeight = smoothstep(-0.16, 0.58, reflection.y);
    vec3 environment = mix(
        vec3(0.43, 0.53, 0.61), vec3(0.60, 0.76, 0.88), skyHeight
    );
    float horizon = 1.0 - smoothstep(0.03, 0.26, abs(reflection.y));
    environment += vec3(0.025, 0.026, 0.022) * horizon;

    float cloud = 0.5 + 0.5 *
        sin(reflection.x * 13.0 + reflection.z * 5.0) *
        sin(reflection.z * 10.0 - reflection.x * 4.0);
    environment += vec3(0.025, 0.028, 0.030) * cloud *
        smoothstep(0.05, 0.50, reflection.y);

    float sunFacing = max(dot(normal, toLight), 0.0);
    float reflectionWeight = clamp(
        0.16 + 0.48 * fresnel + 0.06 * sunFacing, 0.0, 0.68
    );
    vec3 diffuseGlass = baseColor *
        (ambientStrength + diffuseStrength * lambert);
    vec3 color = mix(diffuseGlass, environment, reflectionWeight);

    // A moderately broad sun lobe approximates polished, imperfect glass.
    vec3 halfDirection = normalize(toLight + viewDirection);
    float sun = max(dot(normal, halfDirection), 0.0);
    float specular = 0.12 * pow(sun, 64.0) + 0.025 * pow(sun, 18.0);
    return color + vec3(0.80, 0.88, 0.94) * specular;
}

// Integrate a repeating narrow line over one pixel instead of widening it at
// a distance. This preserves subpixel coverage without harsh grid aliasing.
float facadeLineIntegral(float coordinate, float halfWidth)
{
    float fraction = fract(coordinate);
    return floor(coordinate) * (2.0 * halfWidth)
         + min(fraction, halfWidth)
         + max(fraction - (1.0 - halfWidth), 0.0);
}

float filteredFacadeLine(float coordinate, float halfWidth)
{
    float footprint = max(fwidth(coordinate), 0.0001);
    return clamp((facadeLineIntegral(coordinate + footprint * 0.5, halfWidth)
                - facadeLineIntegral(coordinate - footprint * 0.5, halfWidth))
                / footprint, 0.0, 1.0);
}

vec3 shadeNurAlemGlass(vec3 baseColor, vec3 normal, vec3 toLight, float lambert)
{
    const float pi = 3.14159265359;
    vec2 grid = uv * vec2(96.0, 48.0);
    vec2 panel = floor(grid);
    // Small deterministic differences suggest individual glazing units.
    float variation = fract(sin(dot(panel, vec2(12.9898, 78.233))) * 43758.5453);
    vec3 tint = mix(baseColor, vec3(0.45, 0.65, 0.72), 0.35);
    tint *= 0.975 + 0.05 * variation;
    tint += vec3(-0.006, 0.008, 0.010) * (variation - 0.5);

    // A restrained darker cap hints at the upper energy-system facade zone.
    float upperZone = 1.0 - smoothstep(0.20, 0.30, uv.y);
    tint *= mix(vec3(1.0), vec3(0.82, 0.87, 0.91), upperZone);

    vec3 viewDirection = normalize(worldViewDirection);
    float facing = clamp(dot(normal, viewDirection), 0.0, 1.0);
    float fresnel = 0.04 + 0.96 * pow(1.0 - facing, 5.0);
    vec3 reflection = reflect(-viewDirection, normal);
    vec3 environment = mix(vec3(0.30, 0.38, 0.40), vec3(0.58, 0.76, 0.86),
                           smoothstep(-0.25, 0.65, reflection.y));
    float softCloud = sin(reflection.x * 5.0 + reflection.z * 3.0)
                    * sin(reflection.z * 4.0 - reflection.x * 2.0);
    environment += vec3(0.015, 0.017, 0.018) * softCloud
                 * smoothstep(0.0, 0.55, reflection.y);
    vec3 color = tint * (0.45 + 0.55 * (ambientStrength + diffuseStrength * lambert));
    color = mix(color, environment, 0.22 + 0.38 * fresnel);
    float sun = max(dot(normal, normalize(toLight + viewDirection)), 0.0);
    color += vec3(0.80, 0.89, 0.94) *
        (0.12 * pow(sun, mix(58.0, 72.0, variation)) + 0.015 * pow(sun, 18.0));

    float poleFade = smoothstep(0.025, 0.16, sin(pi * uv.y));
    vec2 fine = vec2(filteredFacadeLine(grid.x, 0.008),
                     filteredFacadeLine(grid.y, 0.006));
    vec2 mainGrid = uv * vec2(24.0, 8.0);
    vec2 mainLines = vec2(filteredFacadeLine(mainGrid.x, 0.0057),
                          filteredFacadeLine(mainGrid.y, 0.0045));
    fine.x *= poleFade;
    mainLines.x *= poleFade;
    float mullion = max(max(mainLines.x, mainLines.y), max(fine.x, fine.y) * 0.60);
    vec3 silver = vec3(0.30, 0.36, 0.39) * (0.55 + 0.45 * lambert);
    silver = mix(silver, environment, 0.12 + 0.14 * fresnel);
    return mix(color, silver, mullion * 0.86);
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

    if (useAsphalt)
    {
        // World-space variation is faint and stable across the street segments.
        float broad = sin(worldPosition.x * 0.43) * sin(worldPosition.z * 0.51);
        float fine = sin(worldPosition.x * 2.1 + worldPosition.z * 1.7);
        baseColor *= 0.99 + 0.018 * broad + 0.006 * fine;
    }

    // lightDirection describes the direction sunlight travels in world space.
    vec3 normal = normalize(worldNormal);
    vec3 toLight = normalize(-lightDirection);
    float lambert = max(dot(normal, toLight), 0.0);
    if (expoCanopyGlass)
    {
        vec3 viewDirection = normalize(worldViewDirection);
        normal = faceforward(normal, -viewDirection, normal);
        float facing = clamp(dot(normal, viewDirection), 0.0, 1.0);
        float fresnel = 0.04 + 0.96 * pow(1.0 - facing, 5.0);
        vec3 reflection = reflect(-viewDirection, normal);
        vec3 environment = mix(vec3(0.69, 0.74, 0.76), vec3(0.55, 0.70, 0.80),
                               smoothstep(-0.12, 0.65, reflection.y));
        vec3 glass = objectColor * (0.82 + 0.18 * max(dot(normal, toLight), 0.0));
        glass = mix(glass, environment, 0.16 + 0.22 * fresnel);
        float highlight = pow(max(dot(normal, normalize(toLight + viewDirection)), 0.0), 64.0);
        // A single layer transmits 74-84% of the background. No emissive term,
        // opaque backing or painted triangular shadow masks the plaza below.
        fragColor = vec4(glass + vec3(0.80, 0.88, 0.94) * highlight * 0.06,
                         0.16 + 0.10 * fresnel);
        return;
    }
    if (greenhouseGlass)
    {
        vec3 viewDirection = normalize(worldViewDirection);
        normal = faceforward(normal, -viewDirection, normal);
        float facing = clamp(dot(normal, viewDirection), 0.0, 1.0);
        float fresnel = 0.04 + 0.96 * pow(1.0 - facing, 5.0);
        vec3 reflection = reflect(-viewDirection, normal);
        // Broad sky/horizon reflection, specific to the conservatory glass.
        // No emissive term or sharp painted streaks; the interior stays visible.
        vec3 sky = mix(vec3(0.48, 0.57, 0.55), vec3(0.57, 0.74, 0.83),
                       smoothstep(-0.18, 0.70, reflection.y));
        float horizon = 1.0 - smoothstep(0.02, 0.35, abs(reflection.y));
        sky += vec3(0.035, 0.036, 0.032) * horizon;
        vec3 glass = objectColor * (ambientStrength + diffuseStrength *
                                    max(dot(normal, toLight), 0.0));
        glass = mix(glass, sky, 0.26 + 0.34 * fresnel);
        float highlight = pow(max(dot(normal, normalize(toLight + viewDirection)), 0.0), 48.0);
        // Two panes still transmit roughly one third of the background face-on.
        float opacity = 0.40 + 0.16 * fresnel;
        fragColor = vec4(glass + vec3(0.82, 0.91, 0.93) * highlight * 0.10, opacity);
        return;
    }
    vec3 color = baseColor * (ambientStrength + diffuseStrength * lambert);

    // The palace's north-facing white stone receives soft daylight fill.
    // The scene's sun direction and all other materials remain unchanged.
    if (akOrdaStone)
    {
        color = baseColor * (0.79 + 0.21 * lambert);
    }

    if (akOrdaRoof)
    {
        // Bright local fill keeps the north-facing blue dome and gold trim
        // legible without altering daylight on the rest of the city.
        vec3 viewDirection = normalize(worldViewDirection);
        vec3 halfDirection = normalize(toLight + viewDirection);
        float highlight = pow(max(dot(normal, halfDirection), 0.0), 64.0);
        color = baseColor * (0.67 + 0.33 * lambert)
              + vec3(0.08, 0.10, 0.12) * highlight;
    }

    if (akOrdaWater)
    {
        vec2 basinPoint = (worldPosition.xz - akOrdaWaterCenter)
            / vec2(10.5, 13.0);
        float radius = length(basinPoint);
        float ripple = sin(radius * 58.0 - sceneTime * 2.5);
        float rippleVisibility = 1.0 - smoothstep(
            0.8, 2.0, fwidth(radius * 58.0)
        );
        float fineRipple = sin(radius * 92.0 + sceneTime * 1.6);
        vec3 water = baseColor * (0.90 + 0.025 * ripple * rippleVisibility
                                + 0.008 * fineRipple * rippleVisibility);
        vec3 viewDirection = normalize(worldViewDirection);
        vec3 halfDirection = normalize(toLight + viewDirection);
        float glint = pow(max(dot(normal, halfDirection), 0.0), 48.0);
        color = water * (0.58 + 0.42 * lambert)
              + vec3(0.07, 0.09, 0.10) * glint;
    }

    if (botanicalLake)
    {
        // Low-amplitude world-space waves keep the shallow lake calm and opaque.
        float broad = sin(worldPosition.x * 0.72 + sceneTime * 0.55)
                    * sin(worldPosition.z * 0.88 - sceneTime * 0.42);
        float finePhase = worldPosition.x * 2.1 + worldPosition.z * 1.4
                        + sceneTime * 0.7;
        float fineFade = 1.0 - smoothstep(0.5, 1.5, fwidth(finePhase));
        vec3 water = baseColor *
            (0.97 + 0.025 * broad + 0.008 * sin(finePhase) * fineFade);
        vec3 viewDirection = normalize(worldViewDirection);
        float fresnel = pow(1.0 - max(dot(normal, viewDirection), 0.0), 3.0);
        vec3 sky = vec3(0.55, 0.72, 0.79);
        vec3 halfDirection = normalize(toLight + viewDirection);
        float glint = pow(max(dot(normal, halfDirection), 0.0), 96.0);
        color = mix(water * (0.72 + 0.28 * lambert), sky,
                    0.08 + 0.16 * fresnel) + vec3(0.07) * glint;
    }

    if (akOrdaWaterJet)
    {
        vec3 viewDirection = normalize(worldViewDirection);
        vec3 halfDirection = normalize(toLight + viewDirection);
        float fresnel = pow(
            1.0 - max(dot(normal, viewDirection), 0.0), 3.0
        );
        float highlight = pow(max(dot(normal, halfDirection), 0.0), 64.0);
        float flow = 0.018 * sin(localPosition.y * 8.0 - sceneTime * 3.0);
        vec3 water = baseColor * (0.88 + 0.12 * lambert + flow);
        color = mix(water, vec3(0.95, 0.98, 1.0),
                    0.06 + 0.16 * fresnel);
        color += vec3(0.07, 0.08, 0.08) * highlight;
    }

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

    if (nurAlemFacade)
    {
        color = shadeNurAlemGlass(baseColor, normal, toLight, lambert);
    }

    if (abuDhabiGlassMaterial)
    {
        color = shadeAbuDhabiGlass(baseColor, normal, toLight, lambert);
    }

    if (abuDhabiFacade)
    {
        // The main tower has narrow bays and floor joints on all four faces.
        // World-space spacing keeps the curtain wall at a consistent scale.
        float across = abs(normal.z) > abs(normal.x) ?
            worldPosition.x : worldPosition.z;
        vec2 grid = vec2(across / 2.25, worldPosition.y / 3.2);
        vec2 edge = min(fract(grid), 1.0 - fract(grid));
        vec2 smoothing = max(fwidth(grid) * 0.65, vec2(0.006));
        float mullion = 1.0 - smoothstep(
            0.025, 0.025 + smoothing.x, edge.x
        );
        float floorJoint = 1.0 - smoothstep(
            0.025, 0.025 + smoothing.y, edge.y
        );
        float detailVisibility = 1.0 - smoothstep(
            0.35, 0.9, max(fwidth(grid.x), fwidth(grid.y))
        );

        vec2 pane = floor(grid);
        float paneTone = 0.985 + 0.015 * sin(
            pane.x * 1.31 + pane.y * 2.07
        );
        color *= paneTone;

        vec3 silver = vec3(0.80, 0.84, 0.86) *
            (0.68 + 0.32 * lambert);
        color = mix(color, silver, mullion * 0.82 * detailVisibility);
        color *= 1.0 - floorJoint * 0.22 * detailVisibility;
    }

    if (abuDhabiSecondaryFacadeMode != 0)
    {
        // Four related curtain-wall rhythms share one antialiased material.
        // The broad midrise uses wider panes and quieter framing.
        float bayWidth = 3.2;
        float floorHeight = 3.5;
        float frameWidth = 0.028;

        if (abuDhabiSecondaryFacadeMode == 2)
        {
            bayWidth = 2.4;
            floorHeight = 3.2;
            frameWidth = 0.038;
        }
        else if (abuDhabiSecondaryFacadeMode == 3)
        {
            bayWidth = 2.8;
            floorHeight = 3.0;
            frameWidth = 0.032;
        }
        else if (abuDhabiSecondaryFacadeMode == 4)
        {
            bayWidth = 2.25;
            floorHeight = 3.4;
            frameWidth = 0.034;
        }
        else if (abuDhabiSecondaryFacadeMode == 5)
        {
            // The six-meter podium uses broad storefront bays and a transom.
            bayWidth = 4.0;
            floorHeight = 3.6;
            frameWidth = 0.032;
        }

        float across = abs(normal.z) > abs(normal.x) ?
            worldPosition.x : worldPosition.z;
        vec2 grid = vec2(across / bayWidth, worldPosition.y / floorHeight);
        vec2 edge = min(fract(grid), 1.0 - fract(grid));
        vec2 smoothing = max(fwidth(grid) * 0.7, vec2(0.008));
        float mullion = 1.0 - smoothstep(
            frameWidth, frameWidth + smoothing.x, edge.x
        );
        float floorLine = 1.0 - smoothstep(
            0.030, 0.030 + smoothing.y, edge.y
        );
        float detailVisibility = 1.0 - smoothstep(
            0.35, 0.9, max(fwidth(grid.x), fwidth(grid.y))
        );

        vec2 pane = floor(grid);
        float paneTone = 0.985 + 0.015 * sin(
            pane.x * 1.43 + pane.y * 2.17 +
            float(abuDhabiSecondaryFacadeMode) * 1.1
        );
        color *= paneTone;

        vec3 silver = vec3(0.79, 0.83, 0.85) *
            (0.70 + 0.30 * lambert);
        float framing = max(mullion * 0.84, floorLine * 0.65);
        color = mix(color, silver, framing * detailVisibility);
    }

    if (talanFacadeMode != 0)
    {
        // Tower U follows the rounded perimeter; podium bays follow each flat wall.
        float bay = talanFacadeMode == 1 ? uv.x :
            (abs(normal.z) > abs(normal.x) ? worldPosition.x : worldPosition.z) / 2.4;
        float floorLevel = worldPosition.y / (talanFacadeMode == 1 ? 2.0 : 1.75);
        vec2 grid = vec2(bay, floorLevel);
        vec2 edge = min(fract(grid), 1.0 - fract(grid));
        vec2 smoothing = max(fwidth(grid) * 0.7, vec2(0.012));
        float mullion = 1.0 - smoothstep(0.045, 0.045 + smoothing.x, edge.x);
        float floorBand = 1.0 - smoothstep(0.045, 0.045 + smoothing.y, edge.y);

        // The new gallery has real modeled frames instead of the tower grid.
        if (talanFacadeMode == 3)
        {
            mullion = 0.0;
            floorBand = 0.0;
        }

        float paneVariation = 1.0 + 0.025 * sin(
            floor(grid.x) * 1.37 + floor(grid.y) * 2.03
        );
        vec3 glass = baseColor * (ambientStrength + diffuseStrength * lambert);
        vec3 viewDirection = normalize(worldViewDirection);
        vec3 reflection = reflect(-viewDirection, normal);
        float fresnel = pow(1.0 - max(dot(normal, viewDirection), 0.0), 5.0);
        vec3 sky = mix(
            vec3(0.72, 0.72, 0.69), vec3(0.53, 0.74, 0.88),
            smoothstep(-0.2, 0.55, reflection.y)
        );
        color = mix(glass * paneVariation, sky, 0.38 + 0.24 * fresnel);

        vec3 halfDirection = normalize(toLight + viewDirection);
        float highlight = pow(max(dot(normal, halfDirection), 0.0), 96.0);
        color += vec3(0.74, 0.85, 0.94) * highlight * 0.20;

        vec3 frame = vec3(0.78, 0.72, 0.59) * (0.60 + 0.40 * lambert);
        color = mix(color, frame, max(mullion, floorBand) * 0.92);
    }

    if (!useObjectColor)
    {
        float fade = smoothstep(550.0, 850.0, length(worldPosition.xz));
        color = mix(color, vec3(0.65, 0.80, 0.92), fade);
    }

    fragColor = vec4(color, 1.0);
}
