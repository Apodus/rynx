#version 330

in vec4 positionPass;
in vec3 normalPass;
in vec2 texCoordPass;
in float height;

uniform sampler2D baseMap0;
uniform sampler2D baseMap1;
uniform sampler2D baseMap2;
uniform sampler2D baseMap3;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragNormal;

void main()
{
	vec3 n = normalize(normalPass);
	
	float weigth0 = 80.0 / (abs(height - 12.0) + 0.01);
	float weigth1 = 80.0 / (abs(height - 20.0) + 0.01);
	float weigth2 = 80.0 / (abs(height - 35.0) + 0.01);
	float weigth3 = 0.0;

	vec4 textureColor = vec4(0.0);
	textureColor += weigth0 * texture(baseMap0, texCoordPass.st);
	textureColor += weigth1 * texture(baseMap1, texCoordPass.st);
	textureColor += weigth2 * texture(baseMap2, texCoordPass.st);
	textureColor += weigth3 * texture(baseMap3, texCoordPass.st);
	textureColor /= (weigth0 + weigth1 + weigth2 + weigth3);

	n = n * 0.5 + 0.5;
	//gl_FragData[0] = textureColor;
	//gl_FragData[1] = vec4(n, 1.0);
	//gl_FragData[2] = positionPass;

	fragColor = textureColor;
	fragNormal = vec4(n, 1.0);
}

