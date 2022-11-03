#version 330 core

uniform sampler2D textureDiffuse0;

in vec2 TexCoord;

out vec4 FragColor;


void main()
{
    FragColor = texture(textureDiffuse0, TexCoord);
}
