#version 430 core

#define PI 3.1415926538

layout(local_size_x = 1, local_size_y = 1) in;
layout(binding = 0) uniform samplerCube cubeIn;
layout(binding = 1, rgba32f) uniform imageCube cubeOut;

vec4 convolution(vec3 normal)
{
    vec3 tangent = normalize(cross(vec3(0, 1, 0), normal));
    vec3 bitangent = normalize(cross(normal, tangent));
    mat3 tbn = mat3(tangent, bitangent, normal);
    float delta = 0.025;
    int count = 0; 
    vec4 value = vec4(0, 0, 0, 0);
    for (float phi = 0; phi < 2 * PI; phi += delta)
    {
        for (float theta = 0; theta < PI / 2; theta += delta)
        {
            vec3 tangentDir = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 dir = tbn * tangentDir;

            value += vec4(texture(cubeIn, dir).rgb * cos(theta) * sin(theta), 1);
            ++count;
        }
    }
    return value * PI / count;
}

void main()
{
    uint outX = gl_GlobalInvocationID.x;
    uint outY = gl_GlobalInvocationID.y;
    float inX = (outX + 0.5) / gl_NumWorkGroups.x * 2 - 1;
    float inY = (outY + 0.5) / gl_NumWorkGroups.y * 2 - 1;

    imageStore(cubeOut, ivec3(outX, outY, 0), convolution(normalize(vec3(1, -inY, -inX)))); // +x
    imageStore(cubeOut, ivec3(outX, outY, 1), convolution(normalize(vec3(-1, -inY, inX)))); // -x
    imageStore(cubeOut, ivec3(outX, outY, 2), convolution(normalize(vec3(inX, 1, inY)))); // +y
    imageStore(cubeOut, ivec3(outX, outY, 3), convolution(normalize(vec3(inX, -1, -inY)))); // -y
    imageStore(cubeOut, ivec3(outX, outY, 4), convolution(normalize(vec3(inX, -inY, 1)))); // +z
    imageStore(cubeOut, ivec3(outX, outY, 5), convolution(normalize(vec3(-inX, -inY, -1)))); // -z
}