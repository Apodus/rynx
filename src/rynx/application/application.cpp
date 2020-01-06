
#include <rynx/application/application.hpp>

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

#include <rynx/tech/profiling.hpp>


void rynx::application::Application::openWindow(int width, int height, std::string name) {
	m_window = std::make_shared<Window>();
	m_window->createWindow(width, height, name);

	m_input = std::make_shared<UserIO>(m_window);
	m_textures = std::make_shared<GPUTextures>();
	m_shaders = std::make_shared<rynx::graphics::shaders>();
	
	m_meshRenderer = std::shared_ptr<MeshRenderer>(new MeshRenderer(m_textures, m_shaders));
	// m_meshRenderer = std::make_shared<MeshRenderer>(m_textures, m_shaders);
	m_textRenderer = std::make_shared<TextRenderer>(m_textures, m_shaders);
}

void rynx::application::Application::loadTextures(std::string path) {
	m_textures->createTextures(path);
}

void rynx::application::Application::startFrame() {
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
