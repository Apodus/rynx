#pragma once

#include <vector>
#include <memory>

#include <rynx/tech/ecs.hpp>

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
				virtual ~iaction() {}
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

				virtual void on_entities_erased(rynx::scheduler::context&, const std::vector<rynx::ecs::id>&) {}

			private:
				virtual void onFrameProcess(rynx::scheduler::context&, float dt) = 0;
				
				std::unique_ptr<rynx::scheduler::barrier> m_barrier;
				std::vector<std::unique_ptr<rynx::scheduler::barrier>> m_dependOn;
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

			void entities_erased(rynx::scheduler::context& context, const std::vector<rynx::ecs::id>& ids) {
				for (auto& ruleset : m_rules) {
					ruleset->on_entities_erased(context, ids);
				}
			}

		private:
			std::vector<std::unique_ptr<iruleset>> m_rules;
		};
	}
}
