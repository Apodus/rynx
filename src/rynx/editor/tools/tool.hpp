
#pragma once

#include <rynx/tech/ecs/id.hpp>
#include <vector>
#include <functional>

namespace rynx {
	namespace scheduler {
		class context;
	}

	namespace editor {
		struct editor_shared_state {
			std::vector<rynx::id> m_selected_ids;
			std::function<void(rynx::id)> m_on_entity_selected;
		};

		class itool {
		public:
			virtual ~itool() { m_editor_state = nullptr; }
			virtual void update(rynx::scheduler::context& ctx) = 0;
			virtual void on_tool_selected() = 0;
			virtual void on_tool_unselected() = 0;

			virtual std::string get_tool_name() = 0;
			virtual std::string get_button_texture() = 0;

			virtual bool operates_on(const std::string& type_name) = 0;

			rynx::id selected_id() const {
				if (m_editor_state->m_selected_ids.size() == 1) {
					return m_editor_state->m_selected_ids.front();
				}
				return rynx::id{ 0 };
			}

			editor_shared_state* m_editor_state = nullptr;
		};
	}
}