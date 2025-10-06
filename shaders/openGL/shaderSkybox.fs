#version 300 es
precision highp float;

in vec3 TexCoords;
uniform samplerCube Texture;

out vec4 colour;

void main()
{
    colour = texture(Texture, TexCoords);
}