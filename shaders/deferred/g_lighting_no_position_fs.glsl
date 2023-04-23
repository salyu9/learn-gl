#version 330 core

in vec2 TexCoords;

uniform sampler2D depthTexture;
uniform sampler2D inputNormal;
uniform sampler2D input1;
uniform sampler2D input2;

struct Light
{
    vec3 position;
    vec3 attenuation;
    vec3 color;
    float range;
};
uniform Light lights[32];
uniform int lightCount;

uniform vec3 viewPos;
uniform vec2 frameSize;
uniform mat4 inverseViewProjection;

out vec4 FragColor;

vec3 reconstructPosition(float projectedZ)
{
    float z = projectedZ * 2.0 - 1.0; // z/w
    vec2 xy = gl_FragCoord.xy / frameSize * 2.0 - 1.0;
    vec4 ndc = vec4(xy, z, 1);
    vec4 posInWorld = inverseViewProjection * ndc;
    vec3 position = posInWorld.xyz / posInWorld.w;
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
    vec3 position = reconstructPosition(texture(depthTexture, TexCoords).r);
    vec3 normal = decodeOctahedral(texture(inputNormal, TexCoords).rg);
    vec3 albedo = texture(input1, TexCoords).rgb;
    vec3 specular = texture(input2, TexCoords).rgb;

    vec3 color = vec3(0);
    for (int i = 0; i < lightCount; ++i)
    {
        vec3 lightDiff = lights[i].position - position;
        float dist = length(lightDiff);
        if (dist > lights[i].range) {
            continue;
        }
        vec3 I = lights[i].color / dot(lights[i].attenuation, vec3(1, dist, dist * dist));
        vec3 lightDir = normalize(lightDiff);
        vec3 viewDir = normalize(viewPos - position);
        vec3 h = normalize(lightDir + viewDir);

        color += max(dot(lightDir, normal), 0) * albedo * I;
        color += pow(max(dot(h, normal), 0), 32) * specular * I;
    }

    FragColor = vec4(color, 1);
}