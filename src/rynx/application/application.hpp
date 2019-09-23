#pragma once

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

#include <memory>
#include <string>

namespace rynx {
	namespace application {

		class Application {
		public:
			Application() {}

			void openWindow(int width = 1000, int height = 1000, std::string name = "Rynx Application") {
				m_window = std::make_shared<Window>();
				m_window->createWindow(width, height, name);

				m_input = std::make_shared<UserIO>(m_window);
				m_textures = std::make_shared<GPUTextures>();
				m_shaders = std::make_shared<Shaders>();
				m_meshRenderer = std::make_shared<MeshRenderer>(m_textures, m_shaders);
				m_textRenderer = std::make_shared<TextRenderer>(m_textures, m_shaders);
			}

			void loadTextures(std::string path) {
				m_textures->createTextures(path);
			}

			void startFrame() {
				m_input->update();
				m_window->pollEvents();
				m_meshRenderer->clearScreen();
			}

			void swapBuffers() {
				rynx_profile("Application", "Swap buffers");
				m_window->swap_buffers();
			}

			std::shared_ptr<Window> window() { return m_window; }
			std::shared_ptr<UserIO> input() { return m_input; }
			
			MeshRenderer& meshRenderer() { return *m_meshRenderer; }
			TextRenderer& textRenderer() { return *m_textRenderer; }

			GPUTextures& textures() { return *m_textures; }
			float aspectRatio() const { return m_window->getAspectRatio(); }
			bool isExitRequested() const { return m_window->shouldClose(); }

		private:
			std::shared_ptr<Window> m_window;
			std::shared_ptr<UserIO> m_input;
			std::shared_ptr<GPUTextures> m_textures;
			std::shared_ptr<Shaders> m_shaders;

			std::shared_ptr<MeshRenderer> m_meshRenderer;
			std::shared_ptr<TextRenderer> m_textRenderer;
		};
	}
}
