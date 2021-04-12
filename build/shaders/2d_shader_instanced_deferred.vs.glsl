#version 430

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 color;
layout(location = 4) in int tex_id;
layout(location = 5) in mat4 model;

uniform int atlasBlocksPerRow;
uniform mat4 view;
uniform mat4 projection;
uniform isamplerBuffer uvIndirect;

out vec2 uv_pass;
out vec4 color_pass;
out vec4 normal_pass;
out vec4 position_pass;

out vec2 uv_min;
out float uv_width;

void main()
{
	mat4 model_view = view * model;
    
	// todo: inverse should only be recalculated once per object instance. not once per vertex. :(
	mat3 normal_rotation = transpose(inverse(mat3(model_view)));
	vec3 world_pos = (model * vec4(position, 1.0)).rgb;
	position_pass = vec4(world_pos, 1.0);
	gl_Position = projection * view * vec4(world_pos, 1);
    
	ivec4 indirectionValues = texelFetch(uvIndirect, tex_id);
	int vt_slot = indirectionValues.x;
	int slotSize = indirectionValues.y;
	vec2 uv_min_values = vec2(
					float(vt_slot % atlasBlocksPerRow) / float(atlasBlocksPerRow),
					float(vt_slot / atlasBlocksPerRow) / float(atlasBlocksPerRow)
				);
	uv_width  = float(slotSize) / float(atlasBlocksPerRow);
	uv_pass = uv;
	uv_min = uv_min_values;
	
	color_pass = color;
	normal_pass = vec4(normalize(normal_rotation * normal), 0.0);
}
