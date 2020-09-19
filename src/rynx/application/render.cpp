
#include <rynx/application/render.hpp>
#include <rynx/graphics/renderer/screenspace.hpp>
#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/framebuffer.hpp>
#include <rynx/application/application.hpp>

#include <rynx/application/visualisation/default_graphics_passes.hpp>

#include <rynx/application/visualisation/lights/omnilights_effect.hpp>
#include <rynx/application/visualisation/lights/directed_lights_effect.hpp>
#include <rynx/application/visualisation/lights/ambient_light_effect.hpp>

#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/graphics/renderer/textrenderer.hpp>
#include <rynx/graphics/opengl.hpp>

#include <memory>

rynx::application::renderer::renderer(rynx::application::Application& application, std::shared_ptr<rynx::camera> camera) : m_application(application), camera(camera) {
	shader_copy_color = application.shaders()->load_shader(
		"fbo_color_to_bb",
		"../shaders/screenspace.vs.glsl",
		"../shaders/screenspace_color_passthrough.fs.glsl"
	);

	shader_copy_color->activate();
	shader_copy_color->uniform("tex_color", 0);

	auto omnilights_handler = std::make_unique<rynx::application::visualisation::omnilights_effect>(application.shaders());
	auto directed_lights_handler = std::make_unique<rynx::application::visualisation::directed_lights_effect>(application.shaders());
	auto ambient_lights_handler = std::make_unique<rynx::application::visualisation::ambient_light_effect>(application.shaders());

	m_ambients = ambient_lights_handler.get();

	lighting_pass = std::make_unique<graphics_step>();
	lighting_pass->add_graphics_step(std::move(omnilights_handler));
	lighting_pass->add_graphics_step(std::move(directed_lights_handler));
	lighting_pass->add_graphics_step(std::move(ambient_lights_handler));

	geometry_pass = rynx::application::visualisation::default_geometry_pass(&application.meshRenderer());
	gpu_textures = application.textures();

	current_internal_resolution_geometry = { 1.0f, 1.0f };
	current_internal_resolution_lighting = { 1.0f, 1.0f };

	application.on_resize([this](size_t width, size_t height) {
		on_resolution_change(width, height);
	});

	auto [width, height] = application.current_window_size();
	on_resolution_change(width, height);
}

void rynx::application::renderer::set_geometry_resolution(float percent_x, float percent_y) {
	current_internal_resolution_geometry = { percent_x, percent_y };
	fbo_world_geometry.reset();
	fbo_world_geometry = rynx::graphics::framebuffer::config()
		.set_default_resolution(static_cast<int>(current_resolution.first * current_internal_resolution_geometry.first), static_cast<int>(current_resolution.second * current_internal_resolution_geometry.second))
		.add_rgba8_target("color")
		.add_rgba8_target("normal")
		.add_float_target("position")
		.add_depth32_target()
		.construct(gpu_textures, "world_geometry");
}

void rynx::application::renderer::set_lights_resolution(float percent_x, float percent_y) {
	current_internal_resolution_lighting = {percent_x, percent_y};
	fbo_lights.reset();
	fbo_lights = rynx::graphics::framebuffer::config()
		.set_default_resolution(static_cast<int>(current_resolution.first * current_internal_resolution_lighting.first), static_cast<int>(current_resolution.second * current_internal_resolution_lighting.second))
		.add_rgba8_target("color")
		.construct(gpu_textures, "world_colorized");
}

void rynx::application::renderer::on_resolution_change(size_t new_size_x, size_t new_size_y) {
	current_resolution = {new_size_x, new_size_y};
	set_geometry_resolution(current_internal_resolution_geometry.first, current_internal_resolution_geometry.second);
	set_lights_resolution(current_internal_resolution_lighting.first, current_internal_resolution_lighting.second);
}

void rynx::application::renderer::execute() {
	{
		rynx_profile("Main", "draw");
		m_application.meshRenderer().setDepthTest(true);

		m_application.meshRenderer().setCamera(camera);
		m_application.textRenderer().setCamera(camera);
		m_application.meshRenderer().cameraToGPU();

		fbo_world_geometry->bind_as_output();
		fbo_world_geometry->clear();

		rynx::graphics::gl::enable_blend_for(0);
		rynx::graphics::gl::blend_func_default_for(0);

		rynx::graphics::gl::disable_blend_for(1);
		rynx::graphics::gl::disable_blend_for(2);

		geometry_pass->execute();
	}

	{
		fbo_world_geometry->bind_as_input();
		fbo_lights->bind_as_output();
		rynx::graphics::screenspace_draws::clear_screen();
		lighting_pass->execute();
	}

	{
		m_application.set_gl_viewport_to_window_dimensions();
		shader_copy_color->activate();
		fbo_lights->bind_as_input();
		rynx::graphics::framebuffer::bind_backbuffer();
		rynx::graphics::screenspace_draws::clear_screen();
		rynx::graphics::screenspace_draws::draw_fullscreen();
	}
}

void rynx::application::renderer::prepare(rynx::scheduler::context* ctx) {
	geometry_pass->prepare(ctx);
	lighting_pass->prepare(ctx);
}

void rynx::application::renderer::light_global_ambient(rynx::floats4 color) {
	static_cast<rynx::application::visualisation::ambient_light_effect*>(m_ambients)->set_global_ambient(color);
}

void rynx::application::renderer::light_global_directed(rynx::floats4 color, rynx::vec3f direction) {
	static_cast<rynx::application::visualisation::ambient_light_effect*>(m_ambients)->set_global_directed(color, direction);
}