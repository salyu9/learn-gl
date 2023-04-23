#version 330 core

in vec2 TexCoords;

uniform sampler2D inputPosition;

out vec4 FragColor;

void main()
{
    vec3 position = texture(inputPosition, TexCoords).rgb;

    FragColor = vec4(position, 1);
}