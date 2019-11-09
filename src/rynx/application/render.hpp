#pragma once

#include <rynx/tech/ecs.hpp>
#include <memory>
#include <vector>

namespace rynx {
	namespace scheduler {
		class context;
	}

	namespace application {
		class renderer {
		public:
			class irenderer {
			public:
				virtual ~irenderer() {}
				virtual void render() = 0;
				virtual void prepare(rynx::scheduler::context* ctx) = 0;
			};

			renderer& addRenderer(std::unique_ptr<irenderer> renderer) {
				m_renderers.emplace_back(std::move(renderer));
				return *this;
			}

			void render() {
				for (auto& renderer : m_renderers) {
					renderer->render();
				}
			}

			void prepare(rynx::scheduler::context* ctx) {
				for (auto& renderer : m_renderers) {
					renderer->prepare(ctx);
				}
			}

		private:
			std::vector<std::unique_ptr<irenderer>> m_renderers;
		};
	}
}
