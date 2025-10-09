#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColour;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

vec3 calcPointlight(vec3 colour)
{
    float dotfrag = dot(inNormal, vec3(0.0, 0.0, 1.0));
    vec3 result = max(0.0, dotfrag) * colour;

    return result;
}
void main()
{
    vec3 colour = texture(texSampler, inTexCoord).rgb;
    outColour = vec4(calcPointlight(colour), 1.0);
}