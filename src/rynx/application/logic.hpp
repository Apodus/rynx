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
				struct unique_name_setter {
					void operator()(iruleset& ruleset, std::string name) {
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

				virtual void on_entities_erased(rynx::scheduler::context&, const std::vector<rynx::ecs::id>&) {}
				virtual void clear(rynx::scheduler::context&) {}
				virtual void serialize(rynx::scheduler::context&, rynx::serialization::vector_writer&) {}
				virtual void deserialize(rynx::scheduler::context&, rynx::serialization::vector_reader&) {}
				
				struct apply_to_all {
				public:
					apply_to_all(iruleset* host) : m_host(host) {}

					rynx::serialization::vector_writer serialize(rynx::scheduler::context& context) {
						return m_host->m_parent->serialize(context);
					}

					void deserialize(rynx::scheduler::context& context, rynx::serialization::vector_reader& reader) {
						return m_host->m_parent->deserialize(context, reader);
					}

					void clear(rynx::scheduler::context& context) {
						m_host->m_parent->clear(context);
					}

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
				std::string m_ruleset_unique_name;

				rynx::binary_config::id m_state_id;
				std::unique_ptr<rynx::scheduler::barrier> m_barrier;
				std::vector<std::unique_ptr<rynx::scheduler::barrier>> m_dependOn;
			};

			logic& add_ruleset(std::unique_ptr<rynx::application::logic::iruleset> ruleset);
			void generate_tasks(rynx::scheduler::context& context, float dt);
			void entities_erased(rynx::scheduler::context& context, const std::vector<rynx::ecs::id>& ids);
			void clear(rynx::scheduler::context& context);
			
			void serialize(rynx::scheduler::context& context, rynx::serialization::vector_writer& out) {
				rynx::unordered_map<std::string, std::vector<char>> serializations;
				for (auto&& ruleset : m_rules) {
					rynx::serialization::vector_writer ruleset_out;
					ruleset->serialize(context, ruleset_out);
					if (!ruleset_out.data().empty()) {
						serializations.emplace(ruleset->m_ruleset_unique_name, std::move(ruleset_out.data()));
					}
				}
				rynx::serialize(serializations, out);
			}
			
			rynx::serialization::vector_writer serialize(rynx::scheduler::context& context) {
				rynx::serialization::vector_writer out;
				serialize(context, out);
				return out;
			}

			void deserialize(rynx::scheduler::context& context, rynx::serialization::vector_reader& in) {
				rynx::unordered_map<std::string, std::vector<char>> serializations;
				rynx::deserialize(serializations, in);
				for (auto&& ruleset : m_rules) {
					auto it = serializations.find(ruleset->m_ruleset_unique_name);
					if (it != serializations.end()) {
						rynx::serialization::vector_reader ruleset_in(it->second);
						ruleset->deserialize(context, ruleset_in);
					}
				}
			}

		private:
			std::vector<std::unique_ptr<rynx::application::logic::iruleset>> m_rules;
		};
	}
}
