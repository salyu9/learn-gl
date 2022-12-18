#version 330 core

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;

uniform vec3 lightPosition;

in struct VS_OUT
{
    vec3 position;
    vec2 texCoord;
    vec3 tbnLightDir;
    vec3 tbnViewDir;
} vsOutput;

out vec4 FragColor;

void main()
{
    vec3 color = texture(diffuseTexture, vsOutput.texCoord).rgb;
    vec3 tbnNormal = normalize(texture(normalTexture, vsOutput.texCoord).rgb * 2 - vec3(1));

    // ambient: 0.1

    vec3 ambient = 0.1 * color;

    // diffuse: 1.0
    vec3 lightDir = normalize(vsOutput.tbnLightDir);
    vec3 diffuse = max(dot(lightDir, tbnNormal), 0) * color;

    // spec: 0.2
    vec3 viewDir = normalize(vsOutput.tbnViewDir);
    vec3 h = normalize(lightDir + viewDir);
    float spec = pow(max(dot(h, tbnNormal), 0), 32.0);
    vec3 specular = vec3(0.2 * spec);
    
    vec3 lightColor = ambient + diffuse + specular;

    FragColor = vec4(lightColor, 1);
}