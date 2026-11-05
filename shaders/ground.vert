#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textureCoordinates;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool akOrdaWaterJet;
uniform float sceneTime;

out vec3 worldPosition;
out vec3 worldNormal;
out vec2 uv;
out vec3 localPosition;
out vec3 worldViewDirection;

void main()
{
    vec3 local = position;
    if (akOrdaWaterJet)
    {
        float sway = 0.035 * sin(sceneTime * 2.2 + position.y * 2.7);
        local.x += sway * (position.y / 3.0);
    }
    vec4 world = model * vec4(local, 1.0);
    worldPosition = world.xyz;
    uv = textureCoordinates;
    localPosition = position;
    worldViewDirection = inverse(view)[3].xyz - world.xyz;

    // Inverse transpose keeps normals correct under nonuniform scaling.
    worldNormal = transpose(inverse(mat3(model))) * normal;
    gl_Position = projection * view * world;
}
