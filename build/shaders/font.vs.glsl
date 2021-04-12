#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec2 textureCoordinate;

uniform int atlasBlocksPerRow;
uniform int tex_id;
uniform mat4 MVPMatrix;
uniform isamplerBuffer uvIndirect;

out vec4 vertexColorInterpolated;
out vec2 texCoord;

void main() {
    gl_Position = MVPMatrix * vec4(vertexPosition, 1.0);
	ivec4 indirectionValues = texelFetch(uvIndirect, tex_id);
	int vt_slot = indirectionValues.x;
	int slotSize = indirectionValues.y;
	float u_min = float(vt_slot % atlasBlocksPerRow) / float(atlasBlocksPerRow);
	float v_min = float(vt_slot / atlasBlocksPerRow) / float(atlasBlocksPerRow);
	float width = float(slotSize) / float(atlasBlocksPerRow);
	texCoord = vec2(u_min, v_min) + textureCoordinate * width;
	vertexColorInterpolated = vertexColor;
}