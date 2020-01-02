#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 color;
layout(location = 4) in mat4 model;

uniform mat4 view;
uniform mat4 projection;

out vec2 uv_pass;
out vec4 color_pass;

void main()
{
    gl_Position =  projection * view * model * vec4(position, 1.0);
    uv_pass = uv;
	color_pass = color;
}