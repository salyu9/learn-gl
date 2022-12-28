#version 430 core

#define PI 3.1415926538

layout(local_size_x = 1, local_size_y = 1) in;

layout(binding = 0) uniform sampler2D texIn;
layout(rgba32f, binding = 1) uniform writeonly imageCube imageOut;

vec2 uvAt(vec3 dir)
{
    float u = atan(dir.z, dir.x) / (PI * 2) + 0.5;
    float v = asin(dir.y) / PI + 0.5;
    return vec2(u, v);
}

void main()
{
    uint outX = gl_GlobalInvocationID.x;
    uint outY = gl_GlobalInvocationID.y;
    float inX = float(outX) / gl_NumWorkGroups.x * 2 - 1;
    float inY = float(outY) / gl_NumWorkGroups.y * 2 - 1;

    imageStore(imageOut, ivec3(outX, outY, 0), texture(texIn, uvAt(normalize(vec3(1, -inY, -inX))))); // +x
    imageStore(imageOut, ivec3(outX, outY, 1), texture(texIn, uvAt(normalize(vec3(-1, -inY, inX))))); // -x
    imageStore(imageOut, ivec3(outX, outY, 2), texture(texIn, uvAt(normalize(vec3(inX, 1, inY))))); // +y
    imageStore(imageOut, ivec3(outX, outY, 3), texture(texIn, uvAt(normalize(vec3(inX, -1, -inY))))); // -y
    imageStore(imageOut, ivec3(outX, outY, 4), texture(texIn, uvAt(normalize(vec3(inX, -inY, 1))))); // +z
    imageStore(imageOut, ivec3(outX, outY, 5), texture(texIn, uvAt(normalize(vec3(-inX, -inY, -1))))); // -z
}