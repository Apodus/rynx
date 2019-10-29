#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;

uniform mat4 mvpMatrix;

out vec2 texCoordPass;

void main()
{
	gl_Position = mvpMatrix * vec4(vertexPosition, 1.0);

	texCoordPass = vertexTexCoord;
}