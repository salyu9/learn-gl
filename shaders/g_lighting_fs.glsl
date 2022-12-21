#version 330 core

in vec2 TexCoords;

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

out vec4 FragColor;

void main()
{
    vec4 v0 = texture(input0, TexCoords);
    vec4 v1 = texture(input1, TexCoords);
    vec4 v2 = texture(input2, TexCoords);

    vec3 position = v0.xyz;
    vec3 normal = normalize(vec3(v0.w, v1.xy));
    vec3 albedo = vec3(v1.zw, v2.x);
    vec3 specular = v2.yzw;

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