#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;
layout(location = 3) in vec2 boneIndex;
layout(location = 4) in vec2 boneWeight;

uniform mat4 bones[22];  // Array of bones that are computed (animated) on the CPU and given to the shader
uniform bool active;
uniform vec3 unitLocation;
uniform float scale;

uniform mat4 mvpMatrix;
uniform mat4 mvMatrix;

out vec2 varTexCoord;
out vec4 positionPass;
out vec3 eyeNormal;

void main()
{
	positionPass = vec4(vertexPosition, 1.0);
	eyeNormal = vertexNormal;
	varTexCoord = vertexTexCoord;

	int index;
	
	// The new position of the vertex is the weighted mean of the rotations and translations done by each bone.
	index = int(boneIndex.x);
	positionPass = (bones[index] * vec4(vertexPosition, 1.0)) * boneWeight.x;
	eyeNormal = vec3(bones[index] * vec4(vertexNormal, 0)) * boneWeight.x;

	index = int(boneIndex.y);
	positionPass += (bones[index] * vec4(vertexPosition, 1.0)) * boneWeight.y;
	eyeNormal += vec3(bones[index] * vec4(vertexNormal, 0)) * boneWeight.y;

	positionPass.xyz *= scale;
	positionPass.xyz += unitLocation;
	
	gl_Position = mvpMatrix * vec4(vertexPosition, 1.0);
	eyeNormal = vec3(mvMatrix * vec4(eyeNormal, 0));
}