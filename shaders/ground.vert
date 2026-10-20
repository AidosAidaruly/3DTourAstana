#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldPosition;

void main()
{
    vec4 world = model * vec4(position, 1.0);
    worldPosition = world.xyz;
    gl_Position = projection * view * world;
}
