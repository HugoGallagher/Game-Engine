#version 450

layout(location=0) in vec2 i_pos;
layout(location=1) in vec3 i_col;

layout(location=0) out vec3 f_col;

layout(binding=0) uniform ubo
{
	vec2 pos;
} u;

void main()
{
	gl_Position = vec4(u.pos + i_pos, 0.0, 1.0);
	f_col = i_col;
}