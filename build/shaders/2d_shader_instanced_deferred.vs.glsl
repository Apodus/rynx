#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 color;
layout(location = 4) in mat4 model;

uniform mat4 view;
uniform mat4 projection;

out vec2 uv_pass;
out vec4 color_pass;
out vec4 normal_pass;
out vec4 position_pass;

void main()
{
	mat4 model_view = view * model;
    
	// todo: inverse should only be recalculated once per object instance. not once per vertex. :(
	mat3 normal_rotation = transpose(inverse(mat3(model_view)));
	vec3 world_pos = (model * vec4(position, 1.0)).rgb;
	position_pass = vec4(world_pos, 1.0);
	gl_Position = projection * view * vec4(world_pos, 1);
    
	uv_pass = uv;
	color_pass = color;
	normal_pass = vec4(normalize(normal_rotation * normal), 0);
}
