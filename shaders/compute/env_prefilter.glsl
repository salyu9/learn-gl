#version 430 core

#define PI 3.1415926538

layout (local_size_x = 1, local_size_y = 1) in;
layout (binding = 0) uniform samplerCube cubeIn;
layout (binding = 1, rgba16f) writeonly uniform imageCube cubeOut;

uniform float roughness;

// ------------- hammersley --------------------
float radicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
vec2 hammersley2d(uint i, uint N) {
    return vec2(float(i)/float(N), radicalInverse_VdC(i));
}

// ---------- importance sample ------------------

const uint SAMPLE_COUNT = 1024u;

vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}  

vec4 convolution(vec3 normal)
{
    vec3 N = normalize(normal);    
    vec3 R = N;
    vec3 V = R;

    float totalWeight = 0.0;   
    vec3 prefilteredColor = vec3(0.0);     
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = hammersley2d(i, SAMPLE_COUNT);
        vec3 H  = importanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            prefilteredColor += texture(cubeIn, L).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }
    return vec4(prefilteredColor / totalWeight, 1.0);
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