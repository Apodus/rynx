#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

uniform int atlasBlocksPerRow;
uniform int tex_id;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform isamplerBuffer uvIndirect;

out vec2 uv_pass;



void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);

	ivec4 indirectionValues = texelFetch(uvIndirect, tex_id);
	int vt_slot = indirectionValues.x;
	int slotSize = indirectionValues.y;
	vec2 uv_min = vec2(
					float(vt_slot % atlasBlocksPerRow) / float(atlasBlocksPerRow),
					float(vt_slot / atlasBlocksPerRow) / float(atlasBlocksPerRow)
					);
	float width = float(slotSize) / float(atlasBlocksPerRow);
	uv_pass = uv_min + uv * width;
}