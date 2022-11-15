#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform float explodeDistance;
uniform float explodeRatio;

in Vertex
{
    vec3 normal;
    vec2 texCoord;
} gs_in[];

out Fragment
{
    vec2 texCoord;
    vec4 color;
} gs_out;

void emit_with(int index)
{
    gl_Position = gl_in[index].gl_Position + vec4(gs_in[index].normal * explodeDistance * explodeRatio, 0);
    gs_out.texCoord = gs_in[index].texCoord;
    gs_out.color = vec4(1, 1, 1, 1 - explodeRatio); // remember to enable blending
}

void main()
{
    emit_with(0);
    EmitVertex();

    emit_with(1);
    EmitVertex();

    emit_with(2);
    EmitVertex();

    EndPrimitive();
}