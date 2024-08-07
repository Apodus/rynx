#version 330

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

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);

	ivec4 indirectionValues = texelFetch(uvIndirect, tex_id);
	int vt_slot = indirectionValues.x;
	int slotSize = indirectionValues.y;
	int tex_uv_mode = indirectionValues.z;
	vec2 uv_min = vec2(
					float(vt_slot % atlasBlocksPerRow) / float(atlasBlocksPerRow),
					float(vt_slot / atlasBlocksPerRow) / float(atlasBlocksPerRow)
					);
	float width = float(slotSize) / float(atlasBlocksPerRow);
	if(tex_uv_mode > 0) {
		vec3 world_pos = (model * vec4(position, 1.0)).rgb;
		uv_pass = world_pos.xy * indirectionValues.w * 0.00001;
	}
	else {
		uv_pass = (uv_min + uv * width) * indirectionValues.w * 0.00001;
	}
	
	color_pass = color;
}