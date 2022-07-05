
#pragma once

#include <rynx/tech/serialization_declares.hpp>
#include <cstdint>

namespace rynx {
	namespace graphics {
		struct mesh_id {
			bool operator == (const mesh_id& other) const = default;
			operator bool() const { return value > 0; }
			int64_t value = 0;
		};
	}

	namespace serialization {
		template<> struct Serialize<rynx::graphics::mesh_id> {
			template<typename IOStream>
			void serialize(const rynx::graphics::mesh_id& meshId, IOStream& writer) {
				writer(meshId.value);
			}

			template<typename IOStream>
			void deserialize(rynx::graphics::mesh_id& meshId, IOStream& reader) {
				reader(meshId.value);
			}
		};
	}
}

namespace std {
	template <class T> struct hash;

	template<>
	struct hash<rynx::graphics::mesh_id> {
		size_t operator()(const rynx::graphics::mesh_id& id) const {
			return id.value;
		}
	};
}