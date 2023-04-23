#version 330 core

out float FragColor;

in vec2 TexCoords;

uniform sampler2D depthTexture;
uniform sampler2D normalTexture;
uniform sampler2D noiseTexture;

uniform mat4 projection;
uniform mat4 inverseProjection;
uniform mat4 normalViewMat;

uniform vec2 frameSize;
uniform vec3 kernel[64];
uniform int kernelSize;
uniform float kernelRadius;
uniform bool useNormal;
uniform bool hasRangeCheck;
uniform float bias;

vec3 reconstructViewPosition(float projectedZ)
{
    float z = projectedZ * 2.0 - 1.0; // z/w
    vec2 xy = gl_FragCoord.xy / frameSize * 2.0 - 1.0;
    vec4 ndc = vec4(xy, z, 1);
    vec4 posInView = inverseProjection * ndc;
    vec3 position = posInView.xyz / posInView.w;
    return position;
}

vec3 decodeOctahedral(vec2 n)
{
    float z = 1 - abs(n.x) - abs(n.y);
    n = z >= 0 ? n : (1 - abs(n.yx)) * sign(n);
    return normalize(vec3(n, z));
}

void main()
{
    vec3 position = reconstructViewPosition(texture(depthTexture, TexCoords).r);

    vec3 randomVec = texture(noiseTexture, TexCoords * frameSize / 4).rgb;

    float occlusion = 0;

    if (useNormal) {
        vec3 normal = (normalViewMat * vec4(decodeOctahedral(texture(normalTexture, TexCoords).rg), 0)).xyz;
        vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
        vec3 bitangent = cross(normal, tangent);
        mat3 tbn = mat3(tangent, bitangent, normal);
        for (int i = 0; i < kernelSize; ++i) {
            vec3 samplePos = position + tbn * kernel[i] * kernelRadius;

            vec4 ndc = projection * vec4(samplePos, 1.0);
            vec3 target = ndc.xyz / ndc.w * 0.5 + 0.5;
            float depth = reconstructViewPosition(texture(depthTexture, target.xy).r).z;
            float rangeCheck = hasRangeCheck ? smoothstep(0.0, 1.0, kernelRadius / abs(position.z - depth)) : 1;
            occlusion += (depth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
        }
        occlusion /= kernelSize;
    }
    else {
        for (int i = 0; i < kernelSize; ++i) {
            vec3 samplePos = position + kernel[i] * kernelRadius;

            vec4 ndc = projection * vec4(samplePos, 1.0);
            vec3 target = ndc.xyz / ndc.w * 0.5 + 0.5;
            float depth = reconstructViewPosition(texture(depthTexture, target.xy).r).z;
            float rangeCheck = hasRangeCheck ? smoothstep(0.0, 1.0, kernelRadius / abs(position.z - depth)) : 1;
            occlusion += (depth >= samplePos.z ? 1.0 : 0.0) * rangeCheck;
        }
        occlusion = max(0, occlusion / kernelSize - 0.5);
    }

    FragColor = 1 - occlusion;
}
