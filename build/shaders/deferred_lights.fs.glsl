#version 330

in vec3 fragmentDirection;
in vec2 texCoordPass;

const int MAX_LIGHTS = 71;
uniform vec4 lights[MAX_LIGHTS * 2];
const int POSITION = 0;
const int DIFFUSE = 1;

uniform sampler2D textureColors;
uniform sampler2D normals;
//uniform sampler2D positions;
uniform sampler2D depthTexture;

const int MAX_ACTIVE_LIGHTS = 4;
uniform vec3 cameraPosition;
uniform vec3 cameraTarget;
uniform float light_count;

uniform mat4 modelViewProjectionMatrixInverse;
uniform mat4 projectionMatrixInverse;

out vec4 fragColor;

vec4 reconstruct_position(vec2 tex)
{
//	return texture(positions, tex);

	float depth = texture(depthTexture, tex).r;
	vec4 ndc = vec4(2.0 * tex.x - 1.0, 2.0 * tex.y - 1.0, 2.0 * depth - 1.0, 1);
	vec4 homogeneous = modelViewProjectionMatrixInverse * ndc;
	return vec4(homogeneous.xyz / homogeneous.w,1);
}

vec4 reconstructEyePosition(vec2 tex)
{
//	return gl_ModelViewMatrix * texture(positions, tex);

	float depth = texture(depthTexture, tex).r;
	vec4 ndc = vec4(2.0 * tex.x - 1.0, 2.0 * tex.y - 1.0, 2.0 * depth - 1.0, 1);
	vec4 homogeneous = projectionMatrixInverse * ndc;
	return vec4(homogeneous.xyz / homogeneous.w,1);
}

void main()
{
	vec4 lightColor = vec4(0,0,0,1);
	vec4 textureColor = texture(textureColors, texCoordPass);
	vec3 normal = texture(normals, texCoordPass).rgb;
	normal = 2.0 * normal - 1.0;

	vec4 fragmentEyePosition = reconstructEyePosition(texCoordPass);

	int light_counti = int(light_count);
	for(int i = 0; i < light_counti; ++i)
	{
		int lightIndex = i;

		vec4 lightDiffuse = lights[lightIndex*2 + DIFFUSE];
		vec4 lightPosition = lights[lightIndex*2 + POSITION];

		vec3 light_direction = vec3(lightPosition - fragmentEyePosition);

		float dist = length(light_direction);
		vec3 lightDir = normalize(light_direction);

		float NdotL = 1.0;
		if(dist > 0.0)
		{
			NdotL = dot(normal, normalize(lightDir));
		}

		if(NdotL > 0.0)
		{
			float att = 1.0 / (1.0 + 0.01 * dist * dist);
			lightColor += lightDiffuse * NdotL * att;
		}
	}
	
	fragColor = textureColor * lightColor;
}

