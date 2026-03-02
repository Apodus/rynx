#version 330

in vec2 texCoord_pass;

uniform sampler2D tex_color;
uniform sampler2D tex_normal;
uniform sampler2D tex_position;

uniform vec4 lights_colors[128];
uniform vec4 lights_positions[128];
uniform vec4 lights_settings[128]; // x=???, y=linear attenuation, z=quadratic attenuation, a=backside lighting (penetrating)
uniform int lights_num;
uniform mat4 view_matrix;
uniform mat4 view_projection;

out vec4 frag_color;

float raymarch_shadow(vec3 frag_pos, vec3 light_pos) {
    // view_projection is uploaded from C++ row-major memory.
    // OpenGL treats it as column-major, meaning view_projection is effectively transposed in GLSL.
    // To correctly multiply it, we must do vector * matrix instead of matrix * vector
    vec4 light_clip = vec4(light_pos, 1.0) * view_projection;
    vec2 light_uv = (light_clip.xy / light_clip.w) * 0.5 + 0.5;
    
    vec4 frag_clip = vec4(frag_pos, 1.0) * view_projection;
    vec2 frag_uv = (frag_clip.xy / frag_clip.w) * 0.5 + 0.5;

    vec2 dir = light_uv - frag_uv;
    float dist = length(dir);
    dir = normalize(dir);
    
    // Direction from fragment to light in WORLD space
    vec3 world_dir_to_light = normalize(light_pos - frag_pos);
    
    // G-Buffer normals are now stored in WORLD space.
    vec3 view_dir_to_light = world_dir_to_light;

    int steps = 24;
    float step_size = dist / float(steps);
    float occlusion = 0.0;

    for (int j = 1; j < steps; ++j) {
        float ray_progress = float(j) / float(steps); // 0.0 near frag, 1.0 near light
        
        vec2 sample_uv = frag_uv + dir * (step_size * float(j));
        vec4 sample_normal_val = texture(tex_normal, sample_uv);
        
        // normal unpacking (0..1) -> (-1..1)
        vec3 sample_normal = normalize(2.0 * sample_normal_val.rgb - 1.0);
        
        // Compare view-space normal with view-space light direction
        float normal_agreement = -dot(sample_normal, view_dir_to_light);
        
        if (normal_agreement > 0.0) {
            // Further samples shadow more heavily to represent objects casting long shadows
            // Ray progress controls distance weighting (higher progress = closer to light = casts longer shadow)
            float distance_weight = mix(0.2, 1.0, ray_progress);
            
            // The more the normal points away from the light, the stronger the shadow
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
	float lighting_global_mul = tex_position_val.a;
	
	vec4 tex_normal_val = texture(tex_normal, uv);
	vec3 fragment_normal = normalize(2.0 * tex_normal_val.rgb - 1.0);
	float lighting_direction_bias = tex_normal_val.a;
	
	vec4 result = vec4(0.0, 0.0, 0.0, 1.0);
	for(int i=0; i<lights_num; ++i) {
		vec3 distance_vector = (lights_positions[i].xyz - fragment_position);
		float distance_sqr = dot(distance_vector, distance_vector);
		float agreement = max(lights_settings[i].w, lighting_direction_bias + dot(fragment_normal, normalize(distance_vector)));
		
		vec4 fragment_light_intensity = vec4(agreement * (material_color.rgb * lights_colors[i].rgb) *
			(lights_colors[i].a * lights_colors[i].a), 0.0);
		
        float shadow_factor = raymarch_shadow(fragment_position, lights_positions[i].xyz);
		result += shadow_factor * lighting_global_mul * fragment_light_intensity / (distance_sqr * lights_settings[i].z + sqrt(distance_sqr) * lights_settings[i].y + 1.0);
	}
	
	frag_color = result;
}
