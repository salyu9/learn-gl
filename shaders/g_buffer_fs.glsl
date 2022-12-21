#version 330 core

in VS_OUTPUT
{
    vec3 position;
    vec3 normal;
    vec2 texCoords;
} fsInput;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

layout (location = 0) out vec4 output0;
layout (location = 1) out vec4 output1;
layout (location = 2) out vec4 output2;

void main()
{
    vec3 position = fsInput.position;
    vec3 normal = fsInput.normal;
    vec3 albedo = texture(diffuseTexture, fsInput.texCoords).rgb;
    vec3 specular = texture(specularTexture, fsInput.texCoords).rgb;

    output0 = vec4(position.xyz, normal.x);
    output1 = vec4(normal.yz, albedo.xy);
    output2 = vec4(albedo.z, specular.xyz);
}