#version 330 core

uniform sampler2D textureDiffuse0;

in Fragment{
    vec2 texCoord;
    vec4 color;
} vs_in;

out vec4 FragColor;

void main()
{
    FragColor = texture(textureDiffuse0, vs_in.texCoord) * vs_in.color;
}
