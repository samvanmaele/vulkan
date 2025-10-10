#version 450

layout(location = 0) in vec3 texCoords;
layout(location = 0) out vec4 colour;

layout(set = 0, binding = 1) uniform samplerCube texSampler;

void main()
{
    colour = texture(texSampler, texCoords);
}