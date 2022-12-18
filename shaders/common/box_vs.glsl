#version 330 core

in vec3 pos;

uniform mat4 projection;
uniform mat4 viewModel;

void main()
{
    gl_Position = projection * viewModel * vec4(pos, 1);
}