#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 normalMat;

out VS_OUTPUT
{
    vec3 normal;
    vec2 texCoords;
} vsOutput;

void main()
{
    vsOutput.normal = normalize((normalMat * vec4(aNormal, 0)).xyz);
    vsOutput.texCoords = aTexCoords;

    gl_Position = projection * view * model * vec4(aPosition, 1);
}