
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

rynx::graphics::screenspace_draws::screenspace_draws(std::shared_ptr<rynx::graphics::shaders> shaders) : m_shaders(shaders) {
	
	{
		std::shared_ptr<shader> screenspace_shader = shaders->load_shader(
			"fbo_color_to_bb",
			"../shaders/screenspace.vs.glsl",
			"../shaders/screenspace_color_passthrough.fs.glsl"
		);

		std::shared_ptr<shader> screenspace_shader_ripple = shaders->load_shader(
			"fbo_color_ripple",
			"../shaders/screenspace.vs.glsl",
			"../shaders/screenspace_ripple.fs.glsl"
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

	}
	
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
}

void rynx::graphics::screenspace_draws::draw_fullscreen() {
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void rynx::graphics::screenspace_draws::blend_mode_cumulative() {
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
}

void rynx::graphics::screenspace_draws::blend_mode_default() {
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
}