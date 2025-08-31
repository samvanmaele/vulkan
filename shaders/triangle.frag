#version 310 es
precision highp float;

layout(location = 0) in vec3 inColour;

layout(location = 0) out vec4 outColour;

void main()
{
    outColour = vec4(inColour, 1.0);
}