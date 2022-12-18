#version 330 core

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;

in struct VS_OUT
{
    vec3 position;
    vec2 texCoord;
    vec3 tbnLightPos;
    vec3 tbnViewPos;
    vec3 tbnFragPos;
} vsOutput;

out vec4 FragColor;

vec2 parallaxMapping(vec2 texCoord, vec3 viewDir, float scale)
{
    const float layers = 10;
    float layerDepth = 1.0 / layers;
    vec2 deltaTexCoord = viewDir.xy * scale / layers;
    float prevLayerDepth = 0.0;
    vec2 prevTexCoord = texCoord;
    float prevDepthValue = texture(depthTexture, prevTexCoord).r;
    if (prevDepthValue < 0.0001) {
        return prevTexCoord;
    }

    float currentLayerDepth = 0.0;
    vec2 currentTexCoord = texCoord;
    float currentDepthValue = prevDepthValue;
    while (currentLayerDepth < currentDepthValue) {
        prevLayerDepth = currentLayerDepth;
        prevTexCoord = currentTexCoord;
        prevDepthValue = currentDepthValue;
        currentLayerDepth += layerDepth;
        currentTexCoord -= deltaTexCoord;
        currentDepthValue = texture(depthTexture, currentTexCoord).r;
    }
    float d1 = abs(prevDepthValue - prevLayerDepth);
    float d2 = abs(currentLayerDepth - currentDepthValue);
    return (prevTexCoord * d2 + currentTexCoord * d1) / (d1 + d2);
}

void main()
{
    vec3 viewDir = normalize(vsOutput.tbnViewPos - vsOutput.tbnFragPos);
    vec2 texCoord = parallaxMapping(vsOutput.texCoord, viewDir, 0.1);
    if(texCoord.x > 1.0 || texCoord.y > 1.0 || texCoord.x < 0.0 || texCoord.y < 0.0) {
        discard;
    }

    vec3 color = texture(diffuseTexture, texCoord).rgb;
    vec3 tbnNormal = normalize(texture(normalTexture, texCoord).rgb * 2 - vec3(1));

    // ambient: 0.1

    vec3 ambient = 0.1 * color;

    // diffuse: 1.0
    vec3 lightDir = normalize(vsOutput.tbnLightPos - vsOutput.tbnFragPos);
    vec3 diffuse = max(dot(lightDir, tbnNormal), 0) * color;

    // spec: 0.2
    vec3 h = normalize(lightDir + viewDir);
    float spec = pow(max(dot(h, tbnNormal), 0), 32.0);
    vec3 specular = vec3(0.2 * spec);
    
    vec3 lightColor = ambient + diffuse + specular;

    FragColor = vec4(lightColor, 1);
}