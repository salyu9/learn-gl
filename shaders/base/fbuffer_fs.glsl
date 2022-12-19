#version 330 core
out vec3 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    FragColor = pow(texture(screenTexture, TexCoords).rgb, vec3(1 / 2.2));
}
