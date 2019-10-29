#version 330

layout (location = 0) in vec2 vertexPosition;

uniform float power;

out vec2 texCoordPass;

// remember that you should draw a screen aligned quad
void main(void)
{
	gl_Position = vec4(vertexPosition, 0.0, 1.0);
	texCoordPass = gl_Position.xy * 0.5 + 0.5;
}