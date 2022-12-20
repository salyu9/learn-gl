#version 330 core

out vec3 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;

uniform float exposure;

void main()
{
    vec3 color = texture(screenTexture, TexCoords).rgb;
    vec3 mapped = vec3(1.0) - exp(-color * exposure);

    FragColor = pow(mapped, vec3(1 / 2.2));
}
