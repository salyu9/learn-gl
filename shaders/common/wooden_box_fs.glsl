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

uniform vec3 viewPosition;
uniform Light lights[4];
uniform bool renderBright;

out vec4 FragColor;
out vec4 BrightColor;

void main()
{
    vec3 diffuse = texture(diffuseTexture, fsInput.texCoords).rgb;
    vec3 specular = texture(specularTexture, fsInput.texCoords).rgb;

    vec3 normal = normalize(fsInput.normal);
    vec3 viewDir = normalize(viewPosition - fsInput.position);
    
    vec3 result = vec3(0);

    for (int i = 0; i < 4; ++i)
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