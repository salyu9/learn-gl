#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec4 aTangent;

out VS_OUT
{
    vec3 position;
    vec3 normal;
    vec2 texCoords;
    vec4 tangent;
} vsOut;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 normalMat;

void main()
{
    vec4 worldPos = model * vec4(aPosition, 1);
    gl_Position = projection * view * worldPos;

    vsOut.position = worldPos.xyz;
    vsOut.normal = normalize((normalMat * vec4(aNormal, 0)).xyz);
    vsOut.texCoords = aTexCoords;
    vsOut.tangent = aTangent;
}