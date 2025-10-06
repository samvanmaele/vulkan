#version 300 es
precision highp float;

layout(location = 0) in vec3 vpos;
layout(location = 1) in vec3 vnorm;
layout(location = 2) in vec2 vtex;

layout (std140) uniform PROJ
{
    mat4 projection;
};
layout (std140) uniform VIEW
{
    mat4 view;
};

uniform mat4 model;

out vec3 fragPos;
out vec3 fragNorm;
out vec2 TexCoords;

void main()
{
    vec4 vertPos = model * vec4(vpos, 1.0);

    gl_Position = projection * view * vertPos;
    fragPos = vertPos.xyz;
    fragNorm = vec3(view * model * vec4(vnorm, 0.0));
    TexCoords = vtex;
}