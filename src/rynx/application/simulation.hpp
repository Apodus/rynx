
#pragma once

#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/tech/ecs.hpp>

namespace rynx {
	namespace application {
		struct simulation {
		public:
			simulation(rynx::scheduler::task_scheduler& scheduler) : m_context(scheduler.make_context()) {
				m_context->set_resource<rynx::ecs>(&m_ecs);
			}

			void generate_tasks() {
				m_logic.generate_tasks(*m_context);
			}

			template<typename T>
			void add_rule_set(std::unique_ptr<T>&& t) {
				m_logic.add_ruleset(std::move(t));
			}

			template<typename T>
			void set_resource(T* t) {
				m_context->set_resource(t);
			}

			rynx::ecs m_ecs;
			rynx::application::logic m_logic;
			rynx::scheduler::context* m_context;
		};
	}
}