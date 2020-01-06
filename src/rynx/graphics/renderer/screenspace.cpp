
#include <rynx/graphics/renderer/screenspace.hpp>
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

	uint32_t vao = 0;
	uint32_t vbo = 0;
}

rynx::graphics::screenspace_draws::screenspace_draws() {
	/*
	{
		std::shared_ptr<shader> screenspace_shader_ripple = shaders->load_shader(
			"fbo_color_ripple",
			"../shaders/screenspace.vs.glsl",
			"../shaders/screenspace_ripple.fs.glsl"
		);

		screenspace_shader_ripple->activate();
		// screenspace_shader_ripple->uniform("tex_color", 0);
		screenspace_shader_ripple->uniform("tex_normal", 1);
	}
	*/

	if (vao == 0) {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
	}
}

void rynx::graphics::screenspace_draws::draw_fullscreen() {
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void rynx::graphics::screenspace_draws::clear_screen() {
	float depth_clear_value = 1.0f;
	glClearBufferfv(GL_DEPTH, 0, &depth_clear_value);

	float color_clear[] = { 0, 0, 0, 0 };
	glClearBufferfv(GL_COLOR, 0, color_clear);
}

void rynx::graphics::screenspace_draws::blend_mode_cumulative() {
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
}

void rynx::graphics::screenspace_draws::blend_mode_default() {
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
}