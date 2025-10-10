#version 450

layout(location = 0) in vec3 vpos;

layout(location = 0) out vec3 TexCoords;

layout(set = 0, binding = 0) uniform ubo1
{
    mat4 view;
    mat4 proj;
    //mat4 viewNoTrans;
};

void main()
{
    TexCoords = vpos;
    vec4 pos = proj * vec4(mat3(view) * vpos, 1.0);
    gl_Position = pos.xyww;
}