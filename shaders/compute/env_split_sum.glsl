#version 430 core

#define PI 3.1415926538

layout(local_size_x = 1, local_size_y = 1) in;
layout (binding = 0) uniform samplerCube cubeIn;
layout (binding = 1, rg16f) uniform image2D texOut;

// vec3 SchlickFresnel(vec3 F0, float hv)
// {
//     return F0 + (1 - F0) * pow(clamp(1 - hv, 0.0, 1.0), 5);
// }

float TrowbridgeReitzNDF(float nh, float alpha)
{
    float a2 = alpha * alpha;
    float b = (nh * nh) * (a2 - 1) + 1;
    return a2 / (PI * b * b);
}

float SmithSubGeometry(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1 - k) + k);
}

float SmithGeometry(float nl, float nv, float k)
{
    return SmithSubGeometry(nl, k) * SmithSubGeometry(nv, k);
}

float GKIBL(float alpha)
{
    return alpha * alpha / 2;
}

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

vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = hammersley2d(i, SAMPLE_COUNT);
        vec3 H  = importanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G = SmithGeometry(NdotL, NdotV, GKIBL(roughness));
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

void main()
{
    // x: nv (cos theta)
    // y: roughness
    float nv = (gl_GlobalInvocationID.x + 0.5) / gl_NumWorkGroups.x;
    float roughness = (gl_GlobalInvocationID.y + 0.5) / gl_NumWorkGroups.y;
    imageStore(texOut, ivec2(gl_GlobalInvocationID.xy), vec4(IntegrateBRDF(nv, roughness), 0, 0));
}