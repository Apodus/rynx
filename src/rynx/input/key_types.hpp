
#pragma once

#include <cstdint>

namespace rynx {
	namespace key {
		// a logical key is a name for an action specified by the application. for example "MoveForward" could be a logical key.
		// a logical key is then mapped to some physical key. multiple logical keys can be mapped to the same physical key.
		// this means that the logical key can stay constant in the application even when rebinding keys. the action stays the same,
		// just the physical key that is bound to that logical action changes - and that happens under the hood. you do not need to keep track.
		struct logical {
			struct hash { size_t operator()(const rynx::key::logical& key) const { return std::hash<int32_t>()(key.action_id); } };
			bool operator == (const rynx::key::logical&) const = default;
			int32_t action_id = -1;
		};

		struct physical {
			struct hash { size_t operator()(const rynx::key::physical& key) const { return std::hash<int32_t>()(key.id); } };
			bool operator == (const rynx::key::physical&) const = default;
			int32_t id = -1;
		};

		static constexpr rynx::key::logical invalid_logical_key{ 0 };
		static constexpr rynx::key::physical invalid_physical_key{ 0 };
	}
}