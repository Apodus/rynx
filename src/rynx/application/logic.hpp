#pragma once

#include <vector>
#include <memory>

namespace rynx {
	class ecs;
	class object_storage;

	namespace scheduler {
		struct barrier;
		class context;
		class task_scheduler;
	}

	namespace input {
		class mapped_input;
	}

	namespace application {
		class logic {
		public:
			struct iaction {
				virtual void apply(rynx::ecs& ecs) const = 0;
			};

			class iruleset {
			public:
				iruleset();
				virtual ~iruleset() {}
				virtual std::vector<std::unique_ptr<iaction>> onInput(rynx::input::mapped_input&, const ecs&) { return {}; }
				
				void process(rynx::scheduler::context& scheduler, float dt);
				rynx::scheduler::barrier barrier() const;

				void depends_on(iruleset& other) { other.required_for(*this); }
				void required_for(iruleset& other);

			private:
				std::unique_ptr<rynx::scheduler::barrier> m_barrier;
				std::vector<std::unique_ptr<rynx::scheduler::barrier>> m_dependOn;
				
				virtual void onFrameProcess(rynx::scheduler::context&, float dt) = 0;
			};

			std::vector<std::unique_ptr<iaction>> onInput(rynx::input::mapped_input& input, const ecs& ecs) const {
				std::vector<std::unique_ptr<iaction>> result;
				for (const auto& ruleset : m_rules) {
					auto actions = ruleset->onInput(input, ecs);
					for (auto&& action : actions) {
						result.emplace_back(std::move(action));
					}
				}
				return result;
			}

			logic& add_ruleset(std::unique_ptr<iruleset> ruleset) {
				m_rules.emplace_back(std::move(ruleset));
				return *this;
			}

			void generate_tasks(rynx::scheduler::context& context, float dt) {
				for (auto& ruleset : m_rules) {
					ruleset->process(context, dt);
				}
			}

		private:
			std::vector<std::unique_ptr<iruleset>> m_rules;
		};
	}
}
