#version 330 core

out vec3 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D ssaoTexture;
uniform float weight;

uniform float exposure;

void main()
{
    vec3 color = texture(screenTexture, TexCoords).rgb;
    float ssao = texture(ssaoTexture, TexCoords).r;

    color = color * mix(1 - weight, 1, ssao);

    vec3 mapped = vec3(1.0) - exp(-color * exposure);

    FragColor = pow(mapped, vec3(1 / 2.2));
}
