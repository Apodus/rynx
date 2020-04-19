
#include <rynx/graphics/opengl.hpp>
#include <rynx/graphics/internal/includes.hpp>

void rynx::graphics::gl::enable_blend() {
	glEnable(GL_BLEND);
}

void rynx::graphics::gl::disable_blend() {
	glDisable(GL_BLEND);
}

void rynx::graphics::gl::enable_blend_for(int render_target) {
	glEnablei(GL_BLEND, render_target);
}

void rynx::graphics::gl::disable_blend_for(int render_target) {
	glDisablei(GL_BLEND, render_target);
}

void rynx::graphics::gl::blend_func_cumulative() {
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
}

void rynx::graphics::gl::blend_func_cumulative_for(int render_target) {
	glBlendEquationSeparatei(render_target, GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparatei(render_target, GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
}

void rynx::graphics::gl::blend_func_default() {
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
}

void rynx::graphics::gl::blend_func_default_for(int render_target) {
	glBlendEquationSeparatei(render_target, GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparatei(render_target, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
}