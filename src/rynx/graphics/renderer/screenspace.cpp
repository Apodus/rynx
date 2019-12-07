
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

rynx::graphics::screenspace_renderer::screenspace_renderer(std::shared_ptr<Shaders> shaders) : m_shaders(shaders) {
	
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

		{
			int32_t error_v = glGetError();
			rynx_assert(error_v == GL_NO_ERROR, "gl error: %d", error_v);
		}

		screenspace_shader->activate();

		{
			int32_t error_v = glGetError();
			rynx_assert(error_v == GL_NO_ERROR, "gl error: %d", error_v);
		}

		screenspace_shader->uniform("tex", 0);
		
		{
			int32_t error_v = glGetError();
			rynx_assert(error_v == GL_NO_ERROR, "gl error: %d", error_v);
		}

		screenspace_shader_ripple->activate();
		screenspace_shader_ripple->uniform("tex", 0);

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

void rynx::graphics::screenspace_renderer::draw_fullscreen() {
	m_shaders->activate_shader("fbo_color_to_bb");
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}
