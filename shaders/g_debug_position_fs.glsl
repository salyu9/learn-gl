#version 330 core

in vec2 TexCoords;

uniform sampler2D depthTexture;
uniform vec2 frameSize;
uniform mat4 inverseViewProjection;

out vec4 FragColor;

void main()
{
    float z = texture(depthTexture, TexCoords).r * 2.0 - 1.0; // z/w
    vec2 xy = gl_FragCoord.xy / frameSize * 2.0 - 1.0;
    vec4 ndc = vec4(xy, z, 1);
    vec4 posInView = inverseViewProjection * ndc;
    vec3 position = posInView.xyz / posInView.w;

    FragColor = vec4(position, 1);
}