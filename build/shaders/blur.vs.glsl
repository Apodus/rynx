#version 330

layout(location = 0) in vec2 position;

out vec2 texCoord;

void main(void)
{
	gl_Position = vec4(position, 0.0, 1.0);
	texCoord = gl_Position.xy * 0.5 + 0.5;
}