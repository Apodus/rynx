
#include <rynx/application/render.hpp>
#include <rynx/graphics/renderer/screenspace.hpp>
#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/framebuffer.hpp>
#include <rynx/application/application.hpp>

#include <rynx/application/visualisation/default_graphics_passes.hpp>

#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/graphics/renderer/textrenderer.hpp>

rynx::application::renderer::renderer(rynx::application::Application& application, std::shared_ptr<Camera> camera) : m_application(application), camera(camera) {
	std::shared_ptr<rynx::graphics::shader> copy_operation = application.shaders()->load_shader(
		"fbo_color_to_bb",
		"../shaders/screenspace.vs.glsl",
		"../shaders/screenspace_color_passthrough.fs.glsl"
	);

	copy_operation->activate();
	copy_operation->uniform("tex_color", 0);

	lighting_pass = rynx::application::visualisation::default_lighting_pass(application.shaders());
	geometry_pass = rynx::application::visualisation::default_geometry_pass(&application.meshRenderer(), camera.get());

	fbo_world_geometry = rynx::graphics::framebuffer::config()
		.set_default_resolution(1920, 1080)
		.add_rgba8_target("color")
		.add_rgba8_target("normal")
		.add_float_target("position")
		.add_depth32_target()
		.construct(application.textures(), "world_geometry");
}

void rynx::application::renderer::execute() {
	{
		rynx_profile("Main", "draw");
		// application.meshRenderer().setDepthTest(true);

		m_application.meshRenderer().setCamera(camera);
		m_application.textRenderer().setCamera(camera);
		m_application.meshRenderer().cameraToGPU();

		fbo_world_geometry->bind_as_output();
		fbo_world_geometry->clear();
		geometry_pass->execute();
	}

	{
		m_application.set_gl_viewport_to_window_dimensions();
		fbo_world_geometry->bind_as_input();
		rynx::graphics::framebuffer::bind_backbuffer();
		rynx::graphics::screenspace_draws::clear_screen();
		lighting_pass->execute();
	}
}

void rynx::application::renderer::prepare(rynx::scheduler::context* ctx) {
	geometry_pass->prepare(ctx);
	lighting_pass->prepare(ctx);
}