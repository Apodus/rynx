#pragma once

#include <rynx/tech/ecs.hpp>
#include <memory>
#include <vector>

namespace rynx {
	namespace application {
		class renderer {
		public:
			class irenderer {
			public:
				virtual ~irenderer() {}
				virtual void render(const rynx::ecs&) = 0;
			};

			renderer& addRenderer(std::unique_ptr<irenderer> renderer) {
				m_renderers.emplace_back(std::move(renderer));
				return *this;
			}

			void render(const ecs& ecs) {
				for (auto& renderer : m_renderers) {
					renderer->render(ecs);
				}
			}

		private:
			std::vector<std::unique_ptr<irenderer>> m_renderers;
		};
	}
}
