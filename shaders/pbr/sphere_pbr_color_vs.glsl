#version 330 core

layout (location = 0) in vec3 aPosition;

out VS_OUT
{
    vec3 position;
    vec3 normal;
    vec2 texCoords;
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
    vsOut.normal = normalize((normalMat * vec4(aPosition, 0)).xyz);
}