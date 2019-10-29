#version 330

in vec2 texCoordPass;

uniform sampler2D skyboxTexture;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragNormal;

void main()
{
	vec4 textureColor = texture(skyboxTexture, texCoordPass.st);
	vec3 normal = vec3(0.0);
	vec4 vertex_position = vec4(0.0);

	normal = normal * 0.5 + 0.5;

	fragColor = textureColor;
	fragNormal = vec4(normal, 1.0);
}

