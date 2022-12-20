#version 330 core

uniform sampler2D image;
uniform float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);
uniform bool horizontal;

in vec2 TexCoords;

out vec4 FragColor;

void main()
{
    vec2 texelSize = 1.0 / textureSize(image, 0);
    vec3 result = texture(image, TexCoords).rgb * weight[0];
    vec2 dir = horizontal ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);
    for (int i = 1; i < 5; ++i) {
        result += texture(image, TexCoords + dir * i).rgb * weight[i];
        result += texture(image, TexCoords - dir * i).rgb * weight[i];
    }
    FragColor = vec4(result, 1.0);
}