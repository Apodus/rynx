
#pragma once

#include <rynx/tech/ecs.hpp>
#include <memory>
#include <vector>

namespace rynx {
	namespace scheduler {
		class context;
	}

	namespace application {
		class igraphics_step {
		public:
			virtual ~igraphics_step() {}
			virtual void execute() = 0;
			virtual void prepare(rynx::scheduler::context* ctx) = 0;
		};

		class graphics_step : public igraphics_step {
		public:
			graphics_step& add_graphics_step(std::unique_ptr<igraphics_step> graphics_step) {
				m_graphics_steps.emplace_back(std::move(graphics_step));
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
			std::vector<std::unique_ptr<igraphics_step>> m_graphics_steps;
		};
	}
}
