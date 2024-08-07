#pragma once

#include <rynx/application/simulation.hpp>
#include <rynx/application/render.hpp>
#include <rynx/math/matrix.hpp>
#include <rynx/std/unordered_map.hpp>
#include <rynx/graphics/camera/camera.hpp>
#include <rynx/std/timer.hpp>

#include <vector>

class Window;

namespace rynx {
	class input;

	namespace graphics {
		class mesh;
		class renderer;
		class text_renderer;
		class GPUTextures;
		class shaders;
		class framebuffer;
	}

	namespace application {
		class DebugVisualization;
		class ApplicationDLL Application {
		public:
			Application();

			virtual void startup_load() = 0;
			virtual void set_resources() = 0;
			virtual void set_simulation_rules() = 0;

			struct frame_processing_functionality {
				virtual ~frame_processing_functionality() {}
				virtual void frame_begin() = 0;
				virtual void menu_input() = 0;
				virtual void menu_frame_execute() = 0;
				virtual void logic_frame_start() = 0;
				virtual void logic_frame_wait_end() = 0;
				virtual void wait_gpu_done_and_swap() = 0;
				virtual void render() = 0;
				virtual void frame_end() = 0;
			};

			struct default_frame_processing_functionality : public frame_processing_functionality {
				default_frame_processing_functionality(rynx::observer_ptr<Application> host);
				virtual void frame_begin() override;
				virtual void menu_input() override;
				virtual void menu_frame_execute() override;
				virtual void logic_frame_start() override;
				virtual void logic_frame_wait_end() override;
				virtual void wait_gpu_done_and_swap() override;
				virtual void render() override;
				virtual void frame_end() override;

				rynx::observer_ptr<Application> m_host;
				rynx::shared_ptr<rynx::graphics::framebuffer> m_fbo_menu;
				rynx::shared_ptr<rynx::camera> m_menuCamera;

				rynx::timer m_wall_clock_frame_timer;
				rynx::timer m_render_timer;
				float m_dt = 0.016f;
			};

			frame_processing_functionality& user() {
				return *m_frame_processing_functionality;
			}

			Application& set_frame_processing(rynx::unique_ptr<frame_processing_functionality> ops) {
				m_frame_processing_functionality = std::move(ops);
				return *this;
			}

			Application& set_default_frame_processing() {
				set_frame_processing(rynx::make_unique<default_frame_processing_functionality>(this));
				return *this;
			}

			void openWindow(int width = 1000, int height = 1000, rynx::string name = "Rynx Application");
			void loadTextures(rynx::string path);
			void startFrame();
			void swapBuffers();

			float aspectRatio() const;
			bool isExitRequested() const;
			void set_gl_viewport_to_window_dimensions() const;
			void on_resize(rynx::function<void(size_t, size_t)> onWindowResize);
			std::pair<size_t, size_t> current_window_size() const;

			rynx::graphics::renderer& renderer() { return *m_renderer; }
			rynx::shared_ptr<Window> window() { return m_window; }
			rynx::shared_ptr<rynx::input> input() { return m_input; }
			rynx::shared_ptr<rynx::graphics::shaders> shaders() { return m_shaders; }
			rynx::shared_ptr<DebugVisualization> debugVis() { return m_debugVisualization; }
			rynx::shared_ptr<rynx::graphics::GPUTextures> textures() { return m_textures; }

			rynx::scheduler::task_scheduler& scheduler() { return m_scheduler; }
			rynx::application::simulation& simulation() { return m_simulation; }
			rynx::scheduler::context* simulation_context() { return m_simulation.m_context.get(); }
			rynx::application::renderer* rendering_steps() { return m_rendering_steps.get(); }

		private:
			rynx::unique_ptr<frame_processing_functionality> m_frame_processing_functionality;
			rynx::unique_ptr<application::renderer> m_rendering_steps;

			rynx::scheduler::task_scheduler m_scheduler;
			rynx::application::simulation m_simulation;
			
			rynx::shared_ptr<DebugVisualization> m_debugVisualization;

			rynx::shared_ptr<Window> m_window;
			rynx::shared_ptr<rynx::input> m_input;
			rynx::shared_ptr<rynx::graphics::GPUTextures> m_textures;
			rynx::shared_ptr<rynx::graphics::shaders> m_shaders;
			rynx::shared_ptr<rynx::graphics::renderer> m_renderer;
			
			rynx::camera m_camera;
			std::vector<rynx::function<void(size_t, size_t)>> m_resizeCallbacks;
			bool m_window_resized = false;
		};
	}
}
