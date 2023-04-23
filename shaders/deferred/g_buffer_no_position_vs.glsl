#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 normalMat;

out VS_OUTPUT
{
    vec2 texCoords;
    mat3 tbn;
} vsOutput;

void main()
{
    vsOutput.texCoords = aTexCoords;
    
    vec3 normal = normalize((normalMat * vec4(aNormal, 0)).xyz);
    vec3 tangent = normalize((model * vec4(aTangent, 0)).xyz);
    vec3 bitangent = normalize((model * vec4(aBitangent, 0)).xyz);
    vsOutput.tbn = mat3(tangent, bitangent, normal);

    gl_Position = projection * view * model * vec4(aPosition, 1);
}