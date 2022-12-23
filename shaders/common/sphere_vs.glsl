#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 projection;
uniform mat4 viewModel;

void main()
{
    gl_Position = projection * viewModel * vec4(position, 1);
}