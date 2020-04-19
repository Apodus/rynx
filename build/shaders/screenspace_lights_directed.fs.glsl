#version 330

in vec2 texCoord_pass;

uniform sampler2D tex_color;
uniform sampler2D tex_normal;
uniform sampler2D tex_position;

uniform vec4 lights_colors[128];
uniform vec4 lights_positions[128];
uniform vec4 lights_directions[128]; // rgb = direction, a = arc
uniform vec4 lights_settings[128]; // x=softness, y=linear attenuation, z=quadratic attenuation, a=backside lighting (penetrating)
uniform int lights_num;

out vec4 frag_color;

void main()
{
	vec2 uv = texCoord_pass;
	vec4 material_color = texture(tex_color, uv);
	vec3 fragment_position = texture(tex_position, uv).rgb;
	vec3 fragment_normal = normalize(2.0 * vec3(texture(tex_normal, uv).rgb) - 1.0);
	
	vec4 result = vec4(0.0, 0.0, 0.0, 1.0);
	for(int i=0; i<lights_num; ++i) {
		vec3 light_dir = lights_directions[i].rgb;
		float light_min_agr = lights_directions[i].a;
		vec3 distance_vector = (lights_positions[i].xyz - fragment_position);
		vec3 distance_unit = normalize(distance_vector);
		float dir_agr = -dot(distance_unit, light_dir) - light_min_agr;
		float soft_agr = clamp(dir_agr * lights_settings[i].x, 0.0, 1.0);
		
		float distance_sqr = dot(distance_vector, distance_vector);
		float agreement = max(lights_settings[i].a, dot(fragment_normal, distance_unit));
		vec4 fragment_color_intensity = vec4(soft_agr * agreement * (material_color.rgb * lights_colors[i].rgb) *
			(lights_colors[i].a * lights_colors[i].a), 0.0);
		result +=  fragment_color_intensity / (distance_sqr * lights_settings[i].z + 1.0);
		result +=  fragment_color_intensity / (sqrt(distance_sqr) * lights_settings[i].y + 1.0);
	}
	
	frag_color = result;
}
