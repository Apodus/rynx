
#include <rynx/application/visualisation/omnilights_effect.hpp>
#include <rynx/graphics/shader/shader.hpp>
#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/renderer/screenspace.hpp>

#include <rynx/application/components.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/tech/profiling.hpp>
#include <rynx/tech/ecs.hpp>

#include <rynx/system/assert.hpp>

rynx::application::visualisation::omnilights_effect::omnilights_effect(
	std::shared_ptr<rynx::graphics::shaders> shader_manager,
	std::shared_ptr<rynx::graphics::screenspace_draws> screenspace
) {
	m_screenspace = screenspace;
	m_lights_shader = shader_manager->load_shader(
		"fbo_lights",
		"../shaders/screenspace.vs.glsl",
		"../shaders/screenspace_lights.fs.glsl"
	);

	m_lights_shader->activate();

	// set texture units used for each geometry data.
	m_lights_shader->uniform("tex_color", 0);
	m_lights_shader->uniform("tex_normal", 1);
	m_lights_shader->uniform("tex_position", 2);
}

void rynx::application::visualisation::omnilights_effect::prepare(rynx::scheduler::context* ctx) {
	m_light_colors.clear();
	m_light_positions.clear();
	m_light_ambients.clear();

	ctx->add_task("lights prepare", [this](rynx::scheduler::task& task_context, rynx::ecs& ecs) {
		ecs.query().for_each([this](
			const rynx::components::position& pos,
			const rynx::components::light_omni& light)
		{
			m_light_colors.emplace_back(light.color);
			m_light_ambients.emplace_back(light.ambient);
			m_light_positions.emplace_back(pos.value);
		});
	});
}

void rynx::application::visualisation::omnilights_effect::render() {
	int num_lights = static_cast<int>(m_light_colors.size());
	
	if (num_lights > 0) {
		m_lights_shader->activate();

		int lights_handled = 0;
		constexpr int max_lights_batch = 128; // should match the gpu memory block defined in shader.

		m_screenspace->blend_mode_cumulative();
		while (lights_handled < num_lights) {
			int lights_this_round = (num_lights - lights_handled) > max_lights_batch ? max_lights_batch : (num_lights - lights_handled);
			m_lights_shader->uniform("lights_colors", m_light_colors.data() + lights_handled, lights_this_round);
			m_lights_shader->uniform("lights_positions", m_light_positions.data() + lights_handled, lights_this_round);
			m_lights_shader->uniform("lights_num", lights_this_round);
			m_screenspace->draw_fullscreen();
			lights_handled += lights_this_round;
		}
		m_screenspace->blend_mode_default();
	}
}
