#version 330

in vec2 texCoord_pass;

uniform sampler2D tex_color;
uniform sampler2D tex_normal;
uniform sampler2D tex_position;

uniform vec4 lights_colors[128];
uniform vec3 lights_positions[128];
uniform vec4 lights_directions[128]; // rgb = direction, a = arc
uniform float lights_softness[128];
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
		vec3 light_dir = lights_directions[i].rgb;
		float light_min_agr = lights_directions[i].a;
		vec3 distance_vector = (lights_positions[i] - fragment_position);
		
		vec3 distance_unit_vec = normalize(distance_vector);
		
		float dir_agr = -dot(distance_unit_vec, light_dir) - light_min_agr;
		float soft = lights_softness[i];
		float soft_agr = clamp(dir_agr * soft, 0.0, 1.0);
		
		float distance_sqr = dot(distance_vector, distance_vector);
		float agreement = max(0.0, dot(fragment_normal, normalize(distance_vector)));
		result += vec4(soft_agr * (agreement + 0.1) * (material_color.rgb * lights_colors[i].rgb) *
			(lights_colors[i].a * lights_colors[i].a) / distance_sqr, 1.0);
	}
	
	frag_color = result;
}
