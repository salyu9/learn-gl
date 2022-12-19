#version 330 core
out vec3 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;

    // Reinhard tone mapping
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));

    FragColor = pow(mapped, vec3(1 / 2.2));
}
