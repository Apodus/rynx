#version 330

in vec2 texCoord;

uniform float blurSize;
uniform sampler2D blurScene;

out vec4 fragColor;

void main(void)
{
	vec4 sum = vec4(0.0);
/*
	sum += texture(blurScene, vec2(texCoord.x - 4.0*blurSize, texCoord.y)) * 0.06;
	sum += texture(blurScene, vec2(texCoord.x - 3.0*blurSize, texCoord.y)) * 0.09;
	sum += texture(blurScene, vec2(texCoord.x - 2.0*blurSize, texCoord.y)) * 0.12;
	sum += texture(blurScene, vec2(texCoord.x - 1.0*blurSize, texCoord.y)) * 0.15;
	sum += texture(blurScene, vec2(texCoord.x               , texCoord.y)) * 0.16;
	sum += texture(blurScene, vec2(texCoord.x + 1.0*blurSize, texCoord.y)) * 0.15;
	sum += texture(blurScene, vec2(texCoord.x + 2.0*blurSize, texCoord.y)) * 0.12;
	sum += texture(blurScene, vec2(texCoord.x + 3.0*blurSize, texCoord.y)) * 0.09;
	sum += texture(blurScene, vec2(texCoord.x + 4.0*blurSize, texCoord.y)) * 0.06;
*/

	sum += texture(blurScene, vec2(texCoord.x - 2.0*blurSize, texCoord.y)) * 0.17;
	sum += texture(blurScene, vec2(texCoord.x - 1.0*blurSize, texCoord.y)) * 0.21;
	sum += texture(blurScene, vec2(texCoord.x               , texCoord.y)) * 0.24;
	sum += texture(blurScene, vec2(texCoord.x + 1.0*blurSize, texCoord.y)) * 0.21;
	sum += texture(blurScene, vec2(texCoord.x + 2.0*blurSize, texCoord.y)) * 0.17;
	
	fragColor = sum;
}
