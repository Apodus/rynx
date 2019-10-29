#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;

uniform mat4 mvpMatrix;
uniform mat3 normalMatrix;

out vec4 positionPass;
out vec2 texCoordPass;
out vec3 normalPass;
out float height;

void main()
{
	positionPass = vec4(vertexPosition, 1.0);
	texCoordPass = vertexTexCoord;
	normalPass = normalize(normalMatrix * vertexNormal);
	
	height = vertexPosition.y;

	gl_Position    = mvpMatrix * vec4(vertexPosition, 1.0);
}

