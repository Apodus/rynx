#version 430

in vec2 uv_pass;
in vec4 color_pass;
in vec4 normal_pass;
in vec4 position_pass;
in float opaqueness_pass;
in vec2 uv_min;
in float uv_width;

uniform sampler2D tex;
uniform float light_direction_bias;
uniform float light_global_multiplier;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 frag_position;
layout(location = 3) out vec4 frag_opaqueness;

void main()
{
	vec2 fragmentUv = uv_min + (uv_pass - floor(uv_pass)) * uv_width;
    frag_color = texture(tex, fragmentUv) * color_pass;
	
	// synthesize Z component so the 2D shape has a rounded 'pillowy' normal map instead of flattening to 0 at the center
	vec3 n = normal_pass.xyz;
	float len2 = dot(n.xy, n.xy);
	if (len2 > 1.0) {
		n.xy = normalize(n.xy);
		n.z = 0.0;
	} else {
		n.z = sqrt(1.0 - len2);
	}
	
	// rescale normal to 0-1 for rgba-texture storage.
	frag_normal = vec4(n * 0.5 + 0.5, light_direction_bias);
	frag_position = vec4(position_pass.rgb, light_global_multiplier);
	frag_opaqueness = vec4(opaqueness_pass, opaqueness_pass, opaqueness_pass, 1.0);
}
