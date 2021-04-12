#version 430

in vec2 uv_pass;
in vec4 color_pass;
in vec4 normal_pass;
in vec4 position_pass;
in vec2 uv_min;
in float uv_width;

uniform sampler2D tex;
uniform float light_direction_bias;
uniform float light_global_multiplier;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 frag_position;

void main()
{
	vec2 fragmentUv = uv_min + (uv_pass - floor(uv_pass)) * uv_width;
    frag_color = texture(tex, fragmentUv) * color_pass;
	
	 // rescale normal to 0-1 for rgba-texture storage. NOTE: the interpolated value will not have length 1!
	frag_normal = vec4(normal_pass.rgb * 0.5 + 0.5, light_direction_bias);
	frag_position = vec4(position_pass.rgb, light_global_multiplier);
}
