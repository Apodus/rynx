#version 330

in vec2 texCoord_pass;

uniform sampler2D tex_color;
uniform sampler2D tex_normal;

uniform vec4 lights_colors[2];
uniform vec3 light_direction;

out vec4 frag_color;

void main()
{
	vec2 uv = texCoord_pass;
	vec4 material_color = texture(tex_color, uv);
	vec3 fragment_normal = 2.0 * texture(tex_normal, uv).rgb - 0.5;
	
	vec4 result = vec4(0.0, 0.0, 0.0, 1.0);
	float agreement = max(0.0, dot(fragment_normal, light_direction));
	result += vec4(agreement * (material_color.rgb * lights_colors[0].rgb) * lights_colors[0].a, 0.0);
	result += vec4((material_color.rgb * lights_colors[1].rgb) * lights_colors[1].a, 0.0);
	
	frag_color = result;
}
