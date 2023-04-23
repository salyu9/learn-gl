#version 330 core

in vec2 TexCoords;

uniform sampler2D depthTexture;
uniform sampler2D inputNormal;
uniform sampler2D input1;
uniform sampler2D input2;

uniform vec3 dirLightColor;
uniform vec3 dirLightDir;
uniform vec3 ambientLightColor;

uniform vec3 viewPos;
uniform vec2 frameSize;
uniform mat4 inverseViewProjection;

uniform bool whiteShading;

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
    vec3 albedo = whiteShading ? vec3(1, 1, 1) : texture(input1, TexCoords).rgb;
    vec3 specular = whiteShading ? vec3(0, 0, 0) : texture(input2, TexCoords).rgb;

    vec3 color = vec3(0);
    
    // dir light
    vec3 lightDir = normalize(dirLightDir);
    vec3 viewDir = normalize(viewPos - position);
    vec3 h = normalize(lightDir + viewDir);

    color += max(dot(lightDir, normal), 0) * albedo * dirLightColor;
    color += pow(max(dot(h, normal), 0), 32) * specular * dirLightColor;

    // ambient
    color += albedo * ambientLightColor;

    FragColor = vec4(color, 1);
}