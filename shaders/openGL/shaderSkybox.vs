#version 450
precision highp float;

layout (std140, binding = 0) uniform PROJ
{
    mat4 projection;
};
layout (std140, binding = 2) uniform VIEWROTATION
{
    mat4 view;
};

in vec3 vpos;
out vec3 TexCoords;

void main()
{
    TexCoords = vpos;
    vec4 pos = projection * view * vec4(vpos, 1.0);
    gl_Position = pos.xyww;
}