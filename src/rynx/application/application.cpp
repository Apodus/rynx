
#include <rynx/application/application.hpp>
#include <rynx/application/visualisation/debug_visualisation.hpp>

#include <rynx/graphics/window/window.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/graphics/renderer/textrenderer.hpp>
#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/camera/camera.hpp>
#include <rynx/input/userio.hpp>

#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/application/render.hpp>

#include <rynx/profiling/profiling.hpp>

// For default processing only
#include <rynx/menu/Component.hpp>
#include <rynx/std/timer.hpp>
#include <rynx/graphics/renderer/screenspace.hpp>

using DefaultProcessingFunctionality = rynx::application::Application::default_frame_processing_functionality;

DefaultProcessingFunctionality::default_frame_processing_functionality(rynx::observer_ptr<Application> host) : m_host(host) {
	m_fbo_menu = rynx::graphics::framebuffer::config()
		.set_default_resolution(1920, 1080)
		.add_rgba8_target("color")
		.construct(m_host->textures(), "menu");

	m_menuCamera = rynx::make_shared<rynx::camera>();
	m_menuCamera->setPosition({ 0, 0, 1 });
	m_menuCamera->setDirection({ 0, 0, -1 });
	m_menuCamera->setUpVector({ 0, 1, 0 });
	m_menuCamera->tick(1.0f);
}

void DefaultProcessingFunctionality::frame_begin() {
	rynx_profile("Main", "start frame");
	m_host->startFrame();
	m_host->set_gl_viewport_to_window_dimensions();
}

void DefaultProcessingFunctionality::menu_input() {
	auto& menuSystem = m_host->simulation_context()->get_resource<rynx::menu::System>();
	rynx::mapped_input& gameInput = m_host->simulation_context()->get_resource<rynx::mapped_input>();

	// auto& root = *menuSystem.root();
	menuSystem.input(gameInput);
}

void DefaultProcessingFunctionality::logic_frame_start() {
	auto& menuSystem = m_host->simulation_context()->get_resource<rynx::menu::System>();
	auto& gameInput = m_host->simulation_context()->get_resource<rynx::mapped_input>();
	auto& scheduler = m_host->scheduler();
	
	auto scoped_inhibitor = menuSystem.inhibit_dedicated_inputs(gameInput);
	{
		rynx_profile("Main", "Construct frame tasks");
		m_host->simulation().generate_tasks(0.016f);
	}

	{
		rynx_profile("Main", "Start scheduler");
		scheduler.start_frame();
	}
}

void DefaultProcessingFunctionality::logic_frame_wait_end() {
	auto& scheduler = m_host->scheduler();

	rynx::timer logic_timer;
	rynx_profile("Main", "Wait for frame end");
	scheduler.wait_until_complete();

	auto logic_time_us = logic_timer.time_since_last_access_us();
	// logic_time.observe_value(swap_time.avg() + logic_time_us / 1000.0f); // down to milliseconds.
}

void DefaultProcessingFunctionality::wait_gpu_done_and_swap() {
	rynx::timer swap_buffers_timer;
	m_host->swapBuffers();
	auto swap_time_us = swap_buffers_timer.time_since_last_access_us();
	// swap_time.observe_value(swap_time_us / 1000.0f);
}

void DefaultProcessingFunctionality::menu_frame_execute() {
	auto& menuSystem = m_host->simulation_context()->get_resource<rynx::menu::System>();
	menuSystem.update(dt, m_host->aspectRatio());
}

void DefaultProcessingFunctionality::render() {
	if (true) {
		rynx::timer render_calls_timer;
		rynx_profile("Main", "graphics");

		{
			rynx_profile("Main", "prepare");
			auto& render = *m_host->rendering_steps();
			render.light_global_ambient({ 1.0f, 1.0f, 1.0f, 1.0f });
			render.light_global_directed({ 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f });
			render.prepare(m_host->simulation().m_context.get());
			m_host->scheduler().start_frame();

			// while waiting for computing to be completed, draw menus.
			{
				rynx_profile("Main", "Menus");
				m_fbo_menu->bind_as_output();
				m_fbo_menu->clear();

				m_host->renderer().setDepthTest(false);

				// 2, 2 is the size of the entire screen (in case of 1:1 aspect ratio) for menu camera. left edge is [-1, 0], top right is [+1, +1], etc.
				// so we make it size 2,2 to cover all of that. and then take aspect ratio into account by dividing the y-size.
				
				auto& menuSystem = m_host->simulation_context()->get_resource<rynx::menu::System>();
				auto& root = *menuSystem.root();
				root.scale_local({ 2 , 2 / m_host->aspectRatio(), 0 });
				m_menuCamera->setProjection(0.01f, 50.0f, m_host->aspectRatio());
				m_menuCamera->rebuild_view_matrix();

				m_host->renderer().setCamera(m_menuCamera);
				m_host->renderer().cameraToGPU();
				root.visualise(m_host->renderer());

				auto num_entities = m_host->simulation().m_ecs->size();
				float info_text_pos_y = +0.1f;
				auto get_min_avg_max = [](rynx::numeric_property<float>& prop) {
					return rynx::to_string(prop.min()) + "/" + rynx::to_string(prop.avg()) + "/" + rynx::to_string(prop.max()) + "ms";
				};
			}

			m_host->scheduler().wait_until_complete();
		}


		{
			rynx_profile("Main", "draw");
			m_host->rendering_steps()->execute();

			// TODO: debug visualisations should be drawn on their own fbo.
			m_host->debugVis()->prepare(m_host->simulation().m_context.get());

			{
				m_host->debugVis()->execute();
			}

			{
				m_host->shaders()->activate_shader("fbo_color_to_bb");
				m_fbo_menu->bind_as_input();
				rynx::graphics::screenspace_draws::draw_fullscreen();
			}
		}
	}
}

void DefaultProcessingFunctionality::frame_end() {

}



rynx::application::Application::Application() : m_simulation(m_scheduler) {
}

void rynx::application::Application::openWindow(int width, int height, rynx::string name) {
	m_window = rynx::make_shared<Window>();
	m_window->on_resize([this](size_t /* width */, size_t /* height */) {
		m_window_resized = true;
	});
	
	m_window->createWindow(width, height, name);
	m_input = rynx::make_shared<rynx::input>(m_window);
	m_textures = rynx::make_shared<rynx::graphics::GPUTextures>();
	m_shaders = rynx::make_shared<rynx::graphics::shaders>();
	m_renderer = rynx::make_shared<rynx::graphics::renderer>(m_textures, m_shaders);
	m_debugVisualization = rynx::make_shared<application::DebugVisualization>(m_renderer);

	this->simulation_context()->set_resource(m_camera);
	m_rendering_steps = rynx::make_unique<rynx::application::renderer>(
		textures(),
		shaders(),
		renderer(),
		this->simulation_context()->get_resource_ptr<rynx::camera>()
	);

	on_resize([this](size_t width, size_t height) {
		rendering_steps()->on_resolution_change(width, height);
	});
	rendering_steps()->on_resolution_change(width, height);
}

void rynx::application::Application::loadTextures(rynx::string path) {
	m_textures->loadTexturesFromPath(path);
}

std::pair<size_t, size_t> rynx::application::Application::current_window_size() const {
	return { m_window->width(), m_window->height() };
}

void rynx::application::Application::on_resize(rynx::function<void(size_t, size_t)> onWindowResize) {
	m_resizeCallbacks.emplace_back(std::move(onWindowResize));
}

void rynx::application::Application::startFrame() {
	if (m_window_resized) {
		m_window_resized = false;
		auto [width, height] = current_window_size();
		for (auto&& cb : m_resizeCallbacks)
			cb(width, height);
	}
	
	m_input->update();
	m_window->pollEvents();
}

void rynx::application::Application::swapBuffers() {
	rynx_profile("Application", "Swap buffers");
	m_window->swap_buffers();
}

float rynx::application::Application::aspectRatio() const { return m_window->getAspectRatio(); }
bool rynx::application::Application::isExitRequested() const { return m_window->shouldClose(); }

void rynx::application::Application::set_gl_viewport_to_window_dimensions() const {
	m_window->set_gl_viewport_to_window_dimensions();
}
