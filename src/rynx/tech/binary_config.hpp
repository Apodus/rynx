#pragma once

#include <rynx/tech/dynamic_bitset.hpp>
#include <atomic>
#include <cstdint>
#include <vector>

namespace rynx {
	struct binary_config {
	private:
		rynx::dynamic_bitset m_state;
		std::atomic<int32_t> m_nextStateId = 0;

	public:
		struct id {
			int32_t m_value = 0;
			binary_config* m_host = nullptr;
			std::vector<binary_config::id> m_children;

			id& include_id(id& other) {
				m_children.emplace_back(other);
				return *this;
			}

			bool operator == (const id& other) const = default;

			id& enable() {
				if(m_host)
					m_host->m_state.reset(m_value);
				
				for (auto&& child : m_children) {
					child.enable();
				}
				return *this;
			}

			id& toggle() {
				if (is_enabled()) {
					return disable();
				}
				else {
					return enable();
				}
			}

			id& disable() {
				if(m_host)
					m_host->m_state.set(m_value);
				
				for (auto&& child : m_children) {
					child.disable();
				}
				return *this;
			}

			bool is_enabled() const {
				bool res = true;
				if(m_host)
					res &= !m_host->m_state.test(m_value);
				
				for (auto&& child : m_children) {
					res &= child.is_enabled();
				}
				return res;
			}
		};

		rynx::binary_config::id generate_state_id() {
			return { m_nextStateId.fetch_add(1), this };
		}
	};
}
