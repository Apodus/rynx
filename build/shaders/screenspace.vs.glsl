#version 330

layout (location = 0) in vec2 vertexPosition;

out vec2 texCoord_pass;

void main(void)
{
	gl_Position = vec4(vertexPosition, 0.0, 1.0);
	texCoord_pass = vertexPosition * 0.5 + 0.5;
}
