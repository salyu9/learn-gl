#version 330 core

in layout(location = 0) vec3 aPosition;
in layout(location = 1) vec3 aNormal;
in layout(location = 2) vec2 aTexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 normalMat;

out VS_OUTPUT
{
    vec3 position;
    vec3 normal;
    vec2 texCoords;
} vsOutput;

void main()
{
    vsOutput.position = (model * vec4(aPosition, 1)).xyz;
    vsOutput.normal = (normalMat * vec4(aNormal, 0)).xyz;
    vsOutput.texCoords = aTexCoords;

    gl_Position = projection * view * model * vec4(aPosition, 1);
}