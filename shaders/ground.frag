#version 330 core

in vec3 worldPosition;
out vec4 fragColor;

uniform bool useObjectColor;
uniform vec3 objectColor;

void main()
{
    if (useObjectColor)
    {
        fragColor = vec4(objectColor, 1.0);
        return;
    }

    vec2 cell = floor(worldPosition.xz);
    float checker = mod(cell.x + cell.y, 2.0);
    vec3 color = mix(vec3(0.12, 0.20, 0.24), vec3(0.19, 0.30, 0.34), checker);
    float fade = smoothstep(200.0, 400.0, length(worldPosition.xz));
    fragColor = vec4(mix(color, vec3(0.025, 0.035, 0.055), fade), 1.0);
}
