#version 330 core

in vec2 TexCoords;

uniform sampler2D depthTexture;
uniform sampler2D input0;
uniform sampler2D input1;
uniform sampler2D input2;

struct Light
{
    vec3 position;
    vec3 attenuation;
    vec3 color;
};
uniform Light lights[32];
uniform int lightCount;

uniform vec3 viewPos;
uniform float exposure;
uniform vec2 frameSize;
uniform mat4 inverseViewProjection;

out vec4 FragColor;

vec3 reconstructPosition()
{
    float z = texture(depthTexture, TexCoords).r * 2.0 - 1.0; // z/w
    vec2 xy = gl_FragCoord.xy / frameSize * 2.0 - 1.0;
    vec4 ndc = vec4(xy, z, 1);
    vec4 posInView = inverseViewProjection * ndc;
    vec3 position = posInView.xyz / posInView.w;
    return position;
}

void main()
{
    vec3 position = reconstructPosition();

    vec3 normal = normalize(texture(input0, TexCoords).rgb);
    vec3 albedo = texture(input1, TexCoords).rgb;
    vec3 specular = texture(input2, TexCoords).rgb;

    vec3 color = vec3(0);
    for (int i = 0; i < lightCount; ++i)
    {
        vec3 lightDiff = lights[i].position - position;
        float dist = length(lightDiff);
        vec3 light = lights[i].color / dot(lights[i].attenuation, vec3(1, dist, dist * dist));
        vec3 lightDir = normalize(lightDiff);
        vec3 viewDir = normalize(viewPos - position);
        vec3 h = normalize(lightDir + viewDir);

        color += max(dot(lightDir, normal), 0) * albedo * light;
        color += pow(max(dot(h, normal), 0), 32) * specular * light;
    }

    FragColor = vec4(color, 1);
}