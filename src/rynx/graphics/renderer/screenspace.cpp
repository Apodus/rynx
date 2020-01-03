
#include <rynx/graphics/renderer/screenspace.hpp>
#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/opengl.hpp>

namespace {
	float vertices[] = {
	-1.0f, +1.0f,
	+1.0f, -1.0f,
	-1.0f, -1.0f,

	-1.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, -1.0f
	};
}

rynx::graphics::screenspace_renderer::screenspace_renderer(std::shared_ptr<rynx::graphics::Shaders> shaders) : m_shaders(shaders) {
	
	{
		std::shared_ptr<Shader> screenspace_shader = shaders->load_shader(
			"fbo_color_to_bb",
			"../shaders/screenspace.vs.glsl",
			"../shaders/screenspace_color_passthrough.fs.glsl"
		);

		std::shared_ptr<Shader> screenspace_shader_ripple = shaders->load_shader(
			"fbo_color_ripple",
			"../shaders/screenspace.vs.glsl",
			"../shaders/screenspace_ripple.fs.glsl"
		);

		std::shared_ptr<Shader> screenspace_lights = shaders->load_shader(
			"fbo_lights",
			"../shaders/screenspace.vs.glsl",
			"../shaders/screenspace_lights.fs.glsl"
		);

		{
			int32_t error_v = glGetError();
			rynx_assert(error_v == GL_NO_ERROR, "gl error: %d", error_v);
		}

		screenspace_shader->activate();

		{
			int32_t error_v = glGetError();
			rynx_assert(error_v == GL_NO_ERROR, "gl error: %d", error_v);
		}

		screenspace_shader->uniform("tex_color", 0);
		// screenspace_shader->uniform("tex_normal", 1);

		{
			int32_t error_v = glGetError();
			rynx_assert(error_v == GL_NO_ERROR, "gl error: %d", error_v);
		}

		screenspace_shader_ripple->activate();
		// screenspace_shader_ripple->uniform("tex_color", 0);
		screenspace_shader_ripple->uniform("tex_normal", 1);

		{
			int32_t error_v = glGetError();
			rynx_assert(error_v == GL_NO_ERROR, "gl error: %d", error_v);
		}



		{
			std::vector<floats4> light_colors = {floats4(1.0f, 0.0f, 1.0f, 150.0f), floats4(1.0f, 1.0f, 1.0f, 50.0f) };
			std::vector<vec3f> light_positions = { vec3f(-150, -50, 0), vec3f(+150, -50, 0) };
			int num_lights = 2;

			screenspace_lights->activate();

			// set texture units used for each geometry data.
			screenspace_lights->uniform("tex_color", 0);
			screenspace_lights->uniform("tex_normal", 1);
			screenspace_lights->uniform("tex_position", 2);

			screenspace_lights->uniform("lights_colors", light_colors.data(), num_lights);
			screenspace_lights->uniform("lights_positions", light_positions.data(), num_lights);
			screenspace_lights->uniform("lights_num", num_lights);
		}
	}
	
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
}

void rynx::graphics::screenspace_renderer::draw_fullscreen() {
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}
