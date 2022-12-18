#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTex;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 normalMat;
uniform vec3 lightPosition;
uniform vec3 viewPosition;

out struct VS_OUT
{
    vec3 position;
    vec2 texCoord;
    vec3 tbnLightPos;
    vec3 tbnViewPos;
    vec3 tbnFragPos;
} vsOutput;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1);
    vsOutput.position = (model * vec4(aPos, 1)).xyz;
    vsOutput.texCoord = aTex;
    
    vec3 t = normalize((model * vec4(aTangent, 0)).xyz);
    vec3 b = normalize((model * vec4(aBitangent, 0)).xyz);
    vec3 n = normalize((normalMat * vec4(aNorm, 0)).xyz);
    mat3 inverseTBN = transpose(mat3(t, b, n));

    vsOutput.tbnLightPos = inverseTBN * normalize(lightPosition);
    vsOutput.tbnViewPos = inverseTBN * normalize(viewPosition);
    vsOutput.tbnFragPos = inverseTBN * normalize(vsOutput.position);
}