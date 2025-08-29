#version 310 es
precision highp float;

layout(location = 0) in vec3 inColour;

layout(location = 0) out vec4 outColour;

void main()
{
    outColour = vec4(1.0, 0.0, 0.0, 1.0);
}