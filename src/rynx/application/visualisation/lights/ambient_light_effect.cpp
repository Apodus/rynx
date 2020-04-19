
#include <rynx/application/visualisation/lights/ambient_light_effect.hpp>
#include <rynx/graphics/shader/shader.hpp>
#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/renderer/screenspace.hpp>
#include <rynx/graphics/opengl.hpp>

#include <rynx/graphics/mesh/math.hpp>
#include <rynx/application/components.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/tech/profiling.hpp>
#include <rynx/tech/ecs.hpp>

#include <rynx/system/assert.hpp>

rynx::application::visualisation::ambient_light_effect::ambient_light_effect(
	std::shared_ptr<rynx::graphics::shaders> shader_manager
) {
	m_lights_shader = shader_manager->load_shader(
		"fbo_light_ambient",
		"../shaders/screenspace.vs.glsl",
		"../shaders/screenspace_lights_ambient.fs.glsl"
	);

	m_lights_shader->activate();

	// set texture units used for each geometry data.
	m_lights_shader->uniform("tex_color", 0);
	m_lights_shader->uniform("tex_normal", 1);
	m_light_direction.x = 1.0;
}

void rynx::application::visualisation::ambient_light_effect::prepare(rynx::scheduler::context*) {
	m_light_colors[0] = floats4(0.4f, 1.0f, 0.8f, 0.01f);
	m_light_colors[1] = floats4(1.0f, 1.0f, 1.0f, 0.01f);
	math::rotateXY(m_light_direction, 0.01f);
}

void rynx::application::visualisation::ambient_light_effect::execute() {
	m_lights_shader->activate();
	rynx::graphics::gl::blend_func_cumulative();
	m_lights_shader->uniform("lights_colors", m_light_colors, 2);
	m_lights_shader->uniform("light_direction", m_light_direction);
	rynx::graphics::screenspace_draws::draw_fullscreen();
	rynx::graphics::gl::blend_func_default();
}
