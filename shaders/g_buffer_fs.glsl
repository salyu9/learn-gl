#version 330 core

in VS_OUTPUT
{
    vec3 normal;
    vec2 texCoords;
} fsInput;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

layout (location = 0) out vec3 outputNormal;
layout (location = 1) out vec3 outputAlbedo;
layout (location = 2) out vec3 outputSpecular;

void main()
{
    vec3 normal = fsInput.normal;
    vec3 albedo = texture(diffuseTexture, fsInput.texCoords).rgb;
    vec3 specular = texture(specularTexture, fsInput.texCoords).rgb;

    outputNormal = normal;
    outputAlbedo = albedo;
    outputSpecular = specular;
}