#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform float level;

void main()
{    
    FragColor = vec4(textureLod(skybox, TexCoords, level).rgb, 1);
}
