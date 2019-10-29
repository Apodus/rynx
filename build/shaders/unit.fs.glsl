#version 330

in vec2 varTexCoord;
in vec4 positionPass;
in vec3 eyeNormal;

uniform vec4 unitColor;
uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragNormal;
layout(location = 2) out vec4 fragPosition;

void main()
{
	vec3 n = normalize(eyeNormal);
	vec4 color = unitColor * texture(tex, varTexCoord);

	n = n * 0.5 + 0.5;

	fragColor = color;
	fragNormal = vec4(n, 1.0);
	fragPosition = positionPass;
}