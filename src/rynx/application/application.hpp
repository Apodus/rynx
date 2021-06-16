#pragma once

#include <rynx/application/simulation.hpp>
#include <rynx/math/matrix.hpp>
#include <rynx/tech/unordered_map.hpp>

#include <memory>
#include <string>
#include <vector>
#include <functional>

class Window;

namespace rynx {
	class input;

	namespace graphics {
		class mesh;
		class renderer;
		class text_renderer;
		class GPUTextures;
		class shaders;
	}

	namespace application {
		class DebugVisualization;
		class Application {
		public:
			Application() : m_simulation(m_scheduler) {}

			virtual void set_resources() = 0;
			virtual void set_simulation_rules() = 0;

			void openWindow(int width = 1000, int height = 1000, std::string name = "Rynx Application");
			void loadTextures(std::string path);
			void startFrame();
			void swapBuffers();

			float aspectRatio() const;
			bool isExitRequested() const;
			void set_gl_viewport_to_window_dimensions() const;
			void on_resize(std::function<void(size_t, size_t)> onWindowResize);
			std::pair<size_t, size_t> current_window_size() const;

			rynx::graphics::renderer& renderer() { return *m_renderer; }
			std::shared_ptr<Window> window() { return m_window; }
			std::shared_ptr<rynx::input> input() { return m_input; }
			std::shared_ptr<rynx::graphics::shaders> shaders() { return m_shaders; }
			std::shared_ptr<DebugVisualization> debugVis() { return m_debugVisualization; }
			std::shared_ptr<rynx::graphics::GPUTextures> textures() { return m_textures; }

			rynx::scheduler::task_scheduler& scheduler() { return m_scheduler; }
			rynx::application::simulation& simulation() { return m_simulation; }
			rynx::scheduler::context* simulation_context() { return m_simulation.m_context.get(); }

		private:
			rynx::scheduler::task_scheduler m_scheduler;
			rynx::application::simulation m_simulation;
			
			std::shared_ptr<DebugVisualization> m_debugVisualization;

			std::shared_ptr<Window> m_window;
			std::shared_ptr<rynx::input> m_input;
			std::shared_ptr<rynx::graphics::GPUTextures> m_textures;
			std::shared_ptr<rynx::graphics::shaders> m_shaders;
			std::shared_ptr<rynx::graphics::renderer> m_renderer;
			
			std::vector<std::function<void(size_t, size_t)>> m_resizeCallbacks;
			bool m_window_resized = false;
		};
	}
}
