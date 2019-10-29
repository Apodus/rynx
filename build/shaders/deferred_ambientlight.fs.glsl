#version 330

in vec2 texCoordPass;

uniform vec4 ambientLight;
uniform sampler2D textureColors;
uniform sampler2D normals;

out vec4 fragColor;

void main()
{
	vec4 lightColor = ambientLight;
	vec4 textureColor = texture2D(textureColors, texCoordPass);
	vec3 normal = texture2D(normals, texCoordPass).rgb;
	normal = 2.0 * normal - 1.0;
	if(length(normal) < 0.5)
	{
		// Skybox is always fully lit.
		fragColor = textureColor;
		return;
	}

	fragColor = textureColor * lightColor;
}

