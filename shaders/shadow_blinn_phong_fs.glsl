#version 330 core

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

in VS_OUTPUT
{
    vec3 position;
    vec3 normal;
    vec2 texCoords;
} fsInput;

struct Light
{
    vec3 position;
    vec3 attenuation;
    vec3 color;
};

struct DirLight
{
    vec3 dir;
    vec3 color;
    mat4 lightSpaceMat;
    sampler2D shadowMap;
};

uniform vec3 viewPosition;
uniform Light lights[4];
uniform int lightCount;
uniform DirLight dirLight;
uniform bool hasDirLight;
uniform vec3 ambientLight;
uniform bool renderBright;

out vec4 FragColor;
out vec4 BrightColor;

float calcShadow(vec4 lightSpacePosition, float bias)
{
    vec3 coords = (lightSpacePosition.xyz / lightSpacePosition.w) * 0.5 + 0.5;
    float currentDepth = coords.z;
    if (currentDepth > 1) {
        return 0;
    }

    // float closestDepth = texture(dirLight.shadowMap, coords.xy).r;    
    // //return currentDepth - bias > closestDepth ? 1 : 0;
    // return currentDepth > closestDepth ? 1 : 0;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(dirLight.shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(dirLight.shadowMap, coords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1 : 0;
        }
    }
    shadow /= 9.0;

    return shadow;

}

void main()
{
    vec3 diffuse = texture(diffuseTexture, fsInput.texCoords).rgb;
    vec3 specular = texture(specularTexture, fsInput.texCoords).rgb;

    vec3 normal = normalize(fsInput.normal);
    vec3 viewDir = normalize(viewPosition - fsInput.position);
    
    vec3 result = vec3(0);

    for (int i = 0; i < lightCount; ++i)
    {
        vec3 lightDiff = lights[i].position - fsInput.position;
        float dist = length(lightDiff);
        vec3 lightDir = normalize(lightDiff);
        vec3 light = lights[i].color / dot(lights[i].attenuation, vec3(1, dist, dist * dist));

        // --- diffuse ---
        result += light * max(dot(lightDir, normal), 0) * diffuse;
        // --- specular ---
        vec3 h = normalize(lightDir + viewDir);
        result += light * pow(max(dot(h, normal), 0), 32.0) * specular;
    }

    // ---- dir light ----
    if (hasDirLight) {
        vec3 lightDir = normalize(dirLight.dir);
        vec3 rawLight = vec3(0);
        rawLight += dirLight.color * max(dot(lightDir, normal), 0) * diffuse;
        vec3 h = normalize(lightDir + viewDir);
        rawLight += dirLight.color * pow(max(dot(h, normal), 0), 32.0) * specular;

        //float bias = max(0.05 * (1 - dot(fsInput.normal, lightDir)), 0.005);
        float bias = 0;
        float shadow = calcShadow(dirLight.lightSpaceMat * vec4(fsInput.position, 1.0), bias);
        result += rawLight * (1.0 - shadow);
    }

    result += ambientLight * diffuse;

    FragColor = vec4(result, 1);
    if (renderBright) {
        float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
        if (brightness > 1.0) {
            BrightColor = vec4(FragColor.rgb, 1.0);
        }
        else {
            BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }
}