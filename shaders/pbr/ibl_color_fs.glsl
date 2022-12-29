#version 330 core

#define PI 3.1415926535897932384626433832795

in VS_OUT
{
    vec3 position;
    vec3 normal;
    vec2 texCoords;
} fsIn;

out vec4 FragColor;

uniform vec3 viewPos;
uniform vec3 albedo;
uniform float metalness;
uniform float roughness;
uniform float ao;
uniform float maxPrefilteredLevel;
uniform samplerCube diffuseEnvCube;
uniform samplerCube prefilteredCube;
uniform sampler2D splitSumTex;

struct Light
{
    vec3 position;
    vec3 flux;
};
uniform Light lights[4];

vec3 SchlickFresnel(vec3 F0, float hv)
{
    return F0 + (1 - F0) * pow(clamp(1 - hv, 0.0, 1.0), 5);
}

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

float GKDirect(float alpha)
{
    return (alpha + 1) * (alpha + 1) / 8;
}

float GKIBL(float alpha)
{
    return alpha * alpha / 2;
}

vec3 pbr(vec3 F0, vec3 n, vec3 l, vec3 v, vec3 albedo, float metalness, float roughness, float geometryK)
{
    float roughness2 = roughness * roughness;
    vec3 h = normalize(l + v);
    float nv = max(dot(n, v), 0);
    float nl = max(dot(n, l), 0);
    float nh = max(dot(n, h), 0);
    float hv = max(dot(h, v), 0);
    vec3 F = SchlickFresnel(F0, hv);
    vec3 num = F
     * TrowbridgeReitzNDF(nh, roughness2)
     * SmithGeometry(nl, nv, geometryK);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;

    float den = 4 * nl * nv + 0.0001;
    vec3 specular = num / den;
    return (kD * albedo / PI + specular) * nl;
}

vec3 SchlickFresnelRoughness(vec3 F0, float nv, float roughness)
{
    return F0 + (max(vec3(1 - roughness), F0) - F0) * pow(clamp(1 - nv, 0, 1), 5);
}

void main()
{
    vec3 n = normalize(fsIn.normal);
    vec3 v = normalize(viewPos - fsIn.position);

    vec3 F0 = mix(vec3(0.04), albedo, metalness);
    vec3 Lo = vec3(0, 0, 0);
    for (int i = 0; i < 4; ++i)
    {   
        vec3 lightVector = lights[i].position - fsIn.position;
        float dist = length(lightVector);
        vec3 l = normalize(lightVector);
        vec3 Li = lights[i].flux / (dist * dist) * max(dot(n, l), 0);

        Lo += pbr(F0, n, l, v, albedo, metalness, roughness, GKDirect(roughness)) * Li;
    }

    float nv = max(dot(n, v), 0);
    vec3 F = SchlickFresnelRoughness(F0, nv, roughness);
    //vec3 F = SchlickFresnel(F0, nv, 0));

    // ambient - diffuse
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metalness);
    vec3 diffuse  = texture(diffuseEnvCube, n).rgb * albedo;
    
    // ambient - specular
    vec3 R = reflect(-v, n);
    vec3 specularPrefilerted = textureLod(prefilteredCube, R, roughness * maxPrefilteredLevel).rgb;
    vec2 splitSum = texture(splitSumTex, vec2(nv, roughness)).rg;
    vec3 specular = specularPrefilerted * (F * splitSum.x + splitSum.y);
    
    Lo += (kD * diffuse + specular) * ao;

    FragColor = vec4(Lo, 1);
}