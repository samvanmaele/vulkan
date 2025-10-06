#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outNormal;

layout(set = 0, binding = 0) uniform ubo1
{
    mat4 view;
    mat4 proj;
};

layout(set = 1, binding = 0) uniform ubo2
{
    mat4 model;
};

void main()
{
    gl_Position = proj * view * model * vec4(inPos, 1.0);
    outNormal = vec3(view * model * vec4(inNormal, 0.0));
    outTexCoord = inTexCoord;
}