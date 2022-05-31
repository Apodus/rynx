
#pragma once

#include <rynx/tech/unordered_map.hpp>

#ifndef _WIN32
namespace std {
	constexpr size_t hardware_destructive_interference_size = 64;
}
#endif

namespace rynx {
	class type_index {
		
		alignas(std::hardware_destructive_interference_size) static std::atomic<uint64_t> runningTypeIndex_global;
		static constexpr uint64_t no_type = ~uint64_t(0);

		// NOTE: As long as we are not using DLLs, this should return the same char* value.
		//       This means we can use the pointer value instead of the actual string representation for map key.
#ifdef _WIN32
		template<typename T> static constexpr char const*const unique_str_for_type() { return __FUNCDNAME__; }
#else
		template<typename T> static constexpr char const*const unique_str_for_type() { return __PRETTY_FUNCTION__; }
#endif
		// all type indices will share values of types. requires static linkage of all use sites.
		template<typename T> uint64_t id_private__global_ids() const {
			static uint64_t type_id_value = runningTypeIndex_global++;
			return type_id_value;
		}

		template<typename T> uint64_t id_private() const {
			return id_private__global_ids<T>();
		}

	public:
		struct virtual_type {
			static constexpr uint64_t invalid = ~uint64_t(0);
			uint64_t type_value = invalid;
			bool operator == (uint64_t other) const noexcept { return other == type_value; }
		};

		type_index() = default;

		virtual_type create_virtual_type() {
			return virtual_type{ runningTypeIndex_global++ };
		}

		template<typename T> uint64_t id() const {
			return id_private<std::remove_cv_t<std::remove_reference_t<T>>>();
		}
	};
}
