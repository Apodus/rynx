#version 330

in vec2 texCoord_pass;

uniform sampler2D tex_color;
uniform sampler2D tex_normal;
uniform sampler2D tex_position;

uniform vec4 lights_colors[2];
uniform vec3 light_direction;

out vec4 frag_color;

void main()
{
	vec2 uv = texCoord_pass;
	vec4 material_color = texture(tex_color, uv);
	
	vec4 tex_normal_val = texture(tex_normal, uv);
	vec3 fragment_normal = normalize(2.0 * vec3(tex_normal_val.rgb) - 1.0);
	
	float direction_bias = tex_normal_val.a;
	float light_global_mul = texture(tex_position, uv).a;
	
	vec4 result = vec4(0.0, 0.0, 0.0, 1.0);
	float agreement = max(0.0, direction_bias + dot(fragment_normal, light_direction));
	result += light_global_mul * vec4(agreement * (material_color.rgb * lights_colors[0].rgb) * lights_colors[0].a, 0.0);
	result += light_global_mul * vec4((material_color.rgb * lights_colors[1].rgb) * lights_colors[1].a, 0.0);
	
	frag_color = result;
}
