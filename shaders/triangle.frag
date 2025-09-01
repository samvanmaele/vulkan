#version 310 es
precision highp float;

layout(location = 0) in vec3 inColour;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColour;


vec3 calcPointlight()
{
    float dotfrag = dot(inNormal, vec3(0.0, 0.0, 1.0));

    vec3 result = max(0.0, dotfrag) * inColour;

    return result;
}
void main()
{
    outColour = vec4(calcPointlight(), 1.0);
}