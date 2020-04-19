#version 330

in vec2 uv_pass;
in vec4 color_pass;
in vec4 normal_pass;
in vec4 position_pass;

uniform sampler2D tex;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 frag_position;

void main()
{
    frag_color = texture(tex, uv_pass) * color_pass;
	
	 // rescale normal to 0-1 for rgba-texture storage. NOTE: the interpolated value will not have length 1!
	frag_normal = vec4(normal_pass.rgb * 0.5 + 0.5, 1.0);
	frag_position = position_pass;
}
