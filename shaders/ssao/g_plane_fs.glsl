#version 330 core

in VS_OUTPUT
{
    vec3 normal;
    vec2 texCoords;
} fsInput;

uniform sampler2D albedoTexture;

layout (location = 0) out vec2 outputNormal;
layout (location = 1) out vec3 outputAlbedo;
layout (location = 2) out vec3 outputSpecular;


vec2 encodeOctahedral(vec3 n)
{
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    return n.z >= 0 ? n.xy : (1 - abs(n.yx)) * sign(n.xy);
}

void main()
{
    outputNormal = encodeOctahedral(normalize(fsInput.normal));
    outputAlbedo = texture(albedoTexture, fsInput.texCoords).rgb;
    outputSpecular = vec3(0, 0, 0);
}