#version 330 core

in VS_OUTPUT
{
    vec3 position;
    vec2 texCoords;
    mat3 tbn;
} fsInput;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D normalTexture;

layout (location = 0) out vec3 outputPosition;
layout (location = 1) out vec2 outputNormal;
layout (location = 2) out vec3 outputAlbedo;
layout (location = 3) out vec3 outputSpecular;

vec2 encodeOctahedral(vec3 n)
{
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    return n.z >= 0 ? n.xy : (1 - abs(n.yx)) * sign(n.xy);
}

void main()
{
    vec3 albedo = texture(diffuseTexture, fsInput.texCoords).rgb;
    vec3 specular = texture(specularTexture, fsInput.texCoords).rgb;
    vec3 normal = normalize(fsInput.tbn * (texture(normalTexture, fsInput.texCoords).rgb * 2 - 1));

    outputPosition = fsInput.position;
    outputNormal = encodeOctahedral(normal);
    outputAlbedo = albedo;
    outputSpecular = specular;
}