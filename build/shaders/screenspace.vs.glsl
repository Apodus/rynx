#version 330

layout (location = 0) in vec2 vertexPosition;
layout (location = 1) in vec3 frustumCornerPosition;

out vec2 texCoordPass;
out vec3 fragmentDirection;

// Remember to draw a screen aligned quad.
void main(void)
{
	gl_Position = vec4(vertexPosition, 0.0, 1.0);
	texCoordPass = gl_Position.xy * 0.5 + 0.5;
	fragmentDirection = frustumCornerPosition;
}