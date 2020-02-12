#version 330

in vec2 texCoord_pass;

uniform sampler2D tex_color;
uniform sampler2D tex_normal;
uniform sampler2D tex_position;

uniform vec4 lights_colors[128];
uniform vec4 lights_positions[128];
uniform int lights_num;

out vec4 frag_color;

void main()
{
	vec2 uv = texCoord_pass;
	vec4 material_color = texture(tex_color, uv);
	vec3 fragment_position = texture(tex_position, uv).rgb;
	vec3 fragment_normal = normalize(2.0 * texture(tex_normal, uv).rgb - 1.0);
	
	vec4 result = vec4(0.0, 0.0, 0.0, 1.0);
	for(int i=0; i<lights_num; ++i) {
		vec3 distance_vector = (lights_positions[i].xyz - fragment_position);
		float distance_sqr = dot(distance_vector, distance_vector) + 1.0;
		float agreement = max(0.0, dot(fragment_normal, normalize(distance_vector)));
		result += vec4((agreement + 0.1) * (material_color.rgb * lights_colors[i].rgb) *
			(lights_colors[i].a * lights_colors[i].a) / distance_sqr, 0.0);
	}
	
	frag_color = result;
}
