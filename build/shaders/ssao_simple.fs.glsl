#version 330

in vec2 texCoordPass;

uniform sampler2D imageTexture;
uniform sampler2D depthTexture;

uniform float screen_width;
uniform float screen_height;

uniform float power;

out vec4 fragColor;

float linearizeDepth(float z)
{
	float n = 0.1; // camera z near
	float f = 200.0; // camera z far
	return (2.0 * n) / (f + n - z * (f - n));
}

float getValue(float real_z, float sample_z)
{
	float val = max(real_z - sample_z, 0.0) * 20.0;
	if(val > 1.0)
		val = max((7.0 - val) / 6.0, 0.0);
	return val;
}

float getDepth(float real_z, vec2 depthTexCoord)
{
	float myZ     = texture(depthTexture, depthTexCoord).r;
	float linearZ = linearizeDepth(myZ);
	return getValue(real_z, linearZ);
}

void main()
{
	fragColor = texture(imageTexture, texCoordPass);
	
	vec2 depthtexcoord = gl_FragCoord.xy / vec2(screen_width, screen_height);
	
	float screen_z  = texture(depthTexture, depthtexcoord).r;
	float z  = linearizeDepth(screen_z);
	
	if(z > 0.99)
	{
		return;
	}
	
	float e = power / 800.0;
	float total_occlusion = 0.0;
	
	for(int i = 1; i < 3; ++i)
	{
		float f = float(i);
		total_occlusion += getDepth(z, depthtexcoord + vec2(+e * f, 0)) / f;
		total_occlusion += getDepth(z, depthtexcoord + vec2(-e * f, 0)) / f;
		total_occlusion += getDepth(z, depthtexcoord + vec2(0, +e * f)) / f;
		total_occlusion += getDepth(z, depthtexcoord + vec2(0, -e * f)) / f;
	}
	
	total_occlusion /= 7.0;

	float multiplier = max(1.0 - total_occlusion, 0.1);
	fragColor *= vec4(multiplier, multiplier, multiplier, 1.0);
}



