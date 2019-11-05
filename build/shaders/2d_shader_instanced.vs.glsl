#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in mat4 model;
layout(location = 6) in vec4 color;

uniform mat4 view;
uniform mat4 projection;

out vec2 texCoord_pass;
out vec4 color_pass;

void main()
{
    gl_Position =  projection * view * model * vec4(position, 1.0);
    texCoord_pass = texCoord;
	color_pass = color;
}