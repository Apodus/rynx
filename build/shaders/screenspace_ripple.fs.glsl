#version 330

in vec2 texCoord_pass;

uniform sampler2D tex;
out vec4 frag_color;

void main()
{
	vec2 uv = texCoord_pass + 0.1 * vec2(sin(100.0 * texCoord_pass.x), sin(100.0 * texCoord_pass.y));
    frag_color = texture(tex, uv);
}