#version 300 es
precision highp float;

in vec2 TexCoords;
in vec3 fragPos;
in vec3 fragNorm;

layout (std140) uniform lighting
{
    vec4 lightcolor;
    vec3 lightposition;
};
layout (std140) uniform cam
{
    vec3 campos;
};

uniform sampler2D material;

layout (location = 0) out vec4 color;

vec3 calcPointlight(vec3 baseTexture)
{
    vec3 result = vec3(0);

    vec3 relLightPos = lightposition - fragPos;
    float distance = length(relLightPos);
    relLightPos = normalize(relLightPos);

    vec3 relCamPos = normalize(campos - fragPos);
    vec3 halfVec = normalize(relLightPos + relCamPos);

    vec3 lightval = lightcolor.rgb * lightcolor.a;
    float distsquared = distance * distance;
    float dotfrag = dot(fragNorm, relLightPos);

    result += lightval * max(0.0, dotfrag) / distsquared * baseTexture;
    result += lightval * pow(max(0.0, dot(fragNorm, halfVec)), 32.0) / distsquared;

    return result;
}

void main()
{
    vec4 baseTex = texture(material, TexCoords);
    vec3 temp = 0.2 * baseTex.rgb;
    temp += calcPointlight(baseTex.rgb);

    color = pow(vec4(temp, baseTex.a), vec4(0.45));
}