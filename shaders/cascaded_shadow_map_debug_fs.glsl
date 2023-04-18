#version 330 core
out vec3 FragColor;

in vec2 TexCoords;

uniform sampler2DArray screenTexture;
uniform int layer;

void main()
{
    float v = texture(screenTexture, vec3(TexCoords, layer)).r;
    FragColor = vec3(v, v, v);
}
