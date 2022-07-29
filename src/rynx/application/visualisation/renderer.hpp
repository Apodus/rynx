
#pragma once

#include <rynx/ecs/ecs.hpp>
#include <vector>

namespace rynx {
	namespace scheduler {
		class context;
	}

	namespace application {
		class ApplicationDLL igraphics_step {
		public:
			virtual ~igraphics_step() {}
			virtual void execute() = 0;
			virtual void prepare(rynx::scheduler::context* ctx) = 0;
		};

		class graphics_step : public igraphics_step {
		public:
			graphics_step& add_graphics_step(rynx::unique_ptr<igraphics_step> graphics_step, bool front = false) {
				if (front) {
					m_graphics_steps.insert(m_graphics_steps.begin(), std::move(graphics_step));
				}
				else {
					m_graphics_steps.emplace_back(std::move(graphics_step));
				}
				return *this;
			}

			void execute() override {
				for (auto& graphics_step : m_graphics_steps) {
					graphics_step->execute();
				}
			}

			void prepare(rynx::scheduler::context* ctx) override {
				for (auto& graphics_step : m_graphics_steps) {
					graphics_step->prepare(ctx);
				}
			}

		private:
			std::vector<rynx::unique_ptr<igraphics_step>> m_graphics_steps;
		};
	}
}
