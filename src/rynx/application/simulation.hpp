
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

			void generate_tasks(float dt) {
				m_logic.generate_tasks(*m_context, dt);
			}

			template<typename T>
			void set_resource(T* t) {
				m_context->set_resource(t);
			}

			void clear() {
				m_logic.clear();
			}

			template<typename T>
			struct ruleset_conf {
				ruleset_conf(simulation* host, rynx::binary_config::id state, std::unique_ptr<T> ruleset) : m_ruleset(std::move(ruleset)), m_host(host) { m_ruleset->state_id(state); }
				~ruleset_conf() { m_host->add_rule_set(std::move(m_ruleset)); }

				T* operator->() { return m_ruleset.get(); }

				operator T& () { return *m_ruleset; }
				operator T* () { return m_ruleset.get(); }

				ruleset_conf<T>& state_id(rynx::binary_config::id state) {
					m_ruleset->state_id(state);
					return *this;
				}

			private:
				std::unique_ptr<T> m_ruleset;
				simulation* m_host;
			};

			struct rule_set_config {
				rynx::binary_config::id state;
				simulation* m_host;

				template<typename T, typename...Args>
				ruleset_conf<T> create(Args&&... args) {
					return ruleset_conf<T>(m_host, state, std::make_unique<T>(std::forward<Args>(args)...));
				}
			};

			rule_set_config rule_set(rynx::binary_config::id state = rynx::binary_config::id()) {
				return rule_set_config{state, this};
			}

			template<typename T>
			void add_rule_set(std::unique_ptr<T>&& t) {
				m_logic.add_ruleset(std::move(t));
			}

			rynx::ecs m_ecs;
			rynx::application::logic m_logic;
			rynx::scheduler::context* m_context;
		};
	}
}