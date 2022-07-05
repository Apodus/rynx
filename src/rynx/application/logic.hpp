#pragma once

#include <rynx/tech/ecs/id.hpp>
#include <rynx/tech/binary_config.hpp>
#include <rynx/tech/std/memory.hpp>
#include <rynx/tech/std/string.hpp>

#include <vector>

namespace rynx {
	class object_storage;
	class mapped_input;

	namespace scheduler {
		struct barrier;
		class context;
		class task_scheduler;
	}

	namespace serialization {
		class vector_writer;
		class vector_reader;
	}

	namespace application {
		class logic {
		public:

			class iruleset {
			public:
				struct unique_name_setter {
					void operator()(iruleset& ruleset, rynx::string name) {
						ruleset.m_ruleset_unique_name = name;
					}
				};

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

				virtual void on_entities_erased(rynx::scheduler::context&, const std::vector<rynx::id>&) {}
				virtual void clear(rynx::scheduler::context&) {}
				virtual void serialize(rynx::scheduler::context&, rynx::serialization::vector_writer&) {}
				virtual void deserialize(rynx::scheduler::context&, rynx::serialization::vector_reader&) {}
				
				struct apply_to_all {
				public:
					apply_to_all(iruleset* host) : m_host(host) {}

					rynx::serialization::vector_writer serialize(rynx::scheduler::context& context);
					void deserialize(rynx::scheduler::context& context, rynx::serialization::vector_reader& reader);
					void clear(rynx::scheduler::context& context);

				private:
					iruleset* m_host;
				};

				apply_to_all all_rulesets() {
					return apply_to_all(this);
				}

			private:
				virtual void onFrameProcess(rynx::scheduler::context&, float dt) = 0;
				

				friend class rynx::application::logic;
				rynx::application::logic* m_parent = nullptr;
				rynx::string m_ruleset_unique_name;

				rynx::binary_config::id m_state_id;
				rynx::unique_ptr<rynx::scheduler::barrier> m_barrier;
				std::vector<rynx::unique_ptr<rynx::scheduler::barrier>> m_dependOn;
			};

			logic& add_ruleset(rynx::unique_ptr<rynx::application::logic::iruleset> ruleset);
			void generate_tasks(rynx::scheduler::context& context, float dt);
			void entities_erased(rynx::scheduler::context& context, const std::vector<rynx::id>& ids);
			void clear(rynx::scheduler::context& context);
			
			void serialize(rynx::scheduler::context& context, rynx::serialization::vector_writer& out);
			rynx::serialization::vector_writer serialize(rynx::scheduler::context& context);
			
			void deserialize(rynx::scheduler::context& context, rynx::serialization::vector_reader& in);

		private:
			std::vector<rynx::unique_ptr<rynx::application::logic::iruleset>> m_rules;
		};
	}
}
