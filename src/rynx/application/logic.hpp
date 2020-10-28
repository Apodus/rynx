#pragma once

#include <vector>
#include <memory>

#include <rynx/tech/ecs.hpp>
#include <rynx/tech/binary_config.hpp>

namespace rynx {
	class ecs;
	class object_storage;
	class mapped_input;

	namespace scheduler {
		struct barrier;
		class context;
		class task_scheduler;
	}

	namespace application {
		class logic {
		public:

			class iruleset {
			public:
				iruleset();
				virtual ~iruleset() {}
				
				void process(rynx::scheduler::context& scheduler, float dt);
				rynx::scheduler::barrier barrier() const;

				void depends_on(iruleset& other) { other.required_for(*this); }
				void required_for(iruleset& other);


				iruleset& state_id(rynx::binary_config::id state) {
					m_state_id = state;
					return *this;
				}
				
				rynx::binary_config::id state_id() const {
					return m_state_id;
				}

				virtual void on_entities_erased(rynx::scheduler::context&, const std::vector<rynx::ecs::id>&) {}
				virtual void clear() {}

			private:
				virtual void onFrameProcess(rynx::scheduler::context&, float dt) = 0;
				
				rynx::binary_config::id m_state_id;
				std::unique_ptr<rynx::scheduler::barrier> m_barrier;
				std::vector<std::unique_ptr<rynx::scheduler::barrier>> m_dependOn;
			};

			logic& add_ruleset(std::unique_ptr<iruleset> ruleset);
			void generate_tasks(rynx::scheduler::context& context, float dt);
			void entities_erased(rynx::scheduler::context& context, const std::vector<rynx::ecs::id>& ids);
			void clear();

		private:
			std::vector<std::unique_ptr<iruleset>> m_rules;
		};
	}
}
