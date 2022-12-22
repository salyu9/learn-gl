#version 330 core

in vec2 TexCoords;

uniform sampler2D normalTexture;

out vec4 FragColor;

void main()
{
    vec2 n = texture(normalTexture, TexCoords).rg;
    float z = 1 - abs(n.x) - abs(n.y);
    n = z >= 0 ? n : (1 - abs(n.yx)) * sign(n);
    vec3 normal = normalize(vec3(n, z));

    FragColor = vec4(normal, 1);
}