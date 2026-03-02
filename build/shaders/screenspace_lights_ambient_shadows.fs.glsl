#version 330

in vec2 texCoord_pass;

uniform sampler2D tex_color;
uniform sampler2D tex_normal;
uniform sampler2D tex_position;

uniform vec4 lights_colors[2]; // [0] = directed, [1] = ambient
uniform vec3 light_direction;
uniform mat4 view_matrix;
uniform mat4 view_projection;

out vec4 frag_color;

float raymarch_shadow(vec3 frag_pos) {
    vec4 frag_clip = view_projection * vec4(frag_pos, 1.0);
    vec2 frag_uv = (frag_clip.xy / frag_clip.w) * 0.5 + 0.5;

    // Project light direction into screen space UV direction
    vec4 dir_clip = view_projection * vec4(light_direction, 0.0);
    vec2 uv_dir = normalize(dir_clip.xy);

    int steps = 24;
    float step_size = 1.0 / 64.0; // Fixed step size for directional light
    float occlusion = 0.0;
    
    vec3 world_dir_to_light = -light_direction; // Light comes from light_direction
    // G-Buffer normals are now stored in WORLD space.
    vec3 view_dir_to_light = world_dir_to_light;

    for (int j = 1; j < steps; ++j) {
        float ray_progress = float(j) / float(steps);
        
        vec2 sample_uv = frag_uv - uv_dir * (step_size * float(j));
        
        if (sample_uv.x < 0.0 || sample_uv.x > 1.0 || sample_uv.y < 0.0 || sample_uv.y > 1.0) break;
        
        vec4 sample_normal_val = texture(tex_normal, sample_uv);
        vec3 sample_normal = normalize(2.0 * sample_normal_val.rgb - 1.0);
        
        float normal_agreement = -dot(sample_normal, view_dir_to_light);
        
        if (normal_agreement > 0.0) {
            float distance_weight = mix(0.2, 1.0, ray_progress);
            occlusion += normal_agreement * distance_weight * 0.5;
            
            if (occlusion >= 1.0) {
                occlusion = 1.0;
                break;
            }
        }
    }
    return clamp(1.0 - occlusion, 0.0, 1.0);
}


void main()
{
	vec2 uv = texCoord_pass;
	vec4 material_color = texture(tex_color, uv);
	
	vec4 tex_position_val = texture(tex_position, uv);
	vec3 fragment_position = tex_position_val.rgb;
	float light_global_mul = tex_position_val.a;
	
	vec4 tex_normal_val = texture(tex_normal, uv);
	vec3 fragment_normal = normalize(2.0 * vec3(tex_normal_val.rgb) - 1.0);
	float direction_bias = tex_normal_val.a;
	
	vec4 result = vec4(0.0, 0.0, 0.0, 1.0);
	float agreement = max(0.0, direction_bias + dot(fragment_normal, light_direction));
    
    float shadow_factor = raymarch_shadow(fragment_position);
    
	result += shadow_factor * light_global_mul * vec4(agreement * (material_color.rgb * lights_colors[0].rgb) * lights_colors[0].a, 0.0);
	result += light_global_mul * vec4((material_color.rgb * lights_colors[1].rgb) * lights_colors[1].a, 0.0);
	
	frag_color = result;
}
