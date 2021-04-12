
#pragma once

#include <rynx/tech/serialization.hpp>
#include <rynx/system/annotate.hpp>
#include <cstdint>
#include <utility>

namespace rynx {
	namespace graphics {
		struct ANNOTATE("transient") texture_id {
			bool operator == (const texture_id& other) const = default;
			int32_t value = 0;
		};
	}

	namespace serialization {
		template<> struct Serialize<rynx::graphics::texture_id> {
			template<typename IOStream>
			void serialize(const rynx::graphics::texture_id&, IOStream&) {}
			
			template<typename IOStream>
			void deserialize(rynx::graphics::texture_id&, IOStream&) {}
		};
	}
}

namespace std {
	template<>
	struct hash<rynx::graphics::texture_id> {
		size_t operator()(const rynx::graphics::texture_id& id) const { return id.value; }
	};
}
