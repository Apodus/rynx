
#pragma once

#include <cstdint>
#include <type_traits>

namespace rynx {
	namespace type_index {
		
		struct used_type {
			used_type* next = nullptr;
			char const* name = nullptr;
			uint64_t* id = nullptr;
		};
		
		namespace internal {
			extern used_type* used_type_list;
		}
#ifndef _WIN64
		template<typename T> static char const* const unique_str_for_t() { return __PRETTY_FUNCTION__; }
#else
		template<typename T> static char const* const unique_str_for_t() { return __FUNCSIG__; }
#endif

		template<typename T> struct id_for_type {
			static uint64_t id;
		};

		template<typename T> uint64_t mark_type_is_used(id_for_type<T> t) {
			static used_type used_type_v { rynx::type_index::internal::used_type_list, unique_str_for_t<T>(), &t.id };
			rynx::type_index::internal::used_type_list = &used_type_v;
			return ~uint64_t(0);
		}

		struct virtual_type {
			static constexpr uint64_t invalid = ~uint64_t(0);
			uint64_t type_value = invalid;
			bool operator == (uint64_t other) const noexcept { return other == type_value; }
		};


		void initialize();
		virtual_type create_virtual_type();

		template<typename T> uint64_t id() {
			using naked_t = std::remove_cv_t<std::remove_reference_t<T>>;
			return id_for_type<naked_t>::id;
		}
	};
}

template<typename T> uint64_t rynx::type_index::id_for_type<T>::id = rynx::type_index::mark_type_is_used<T>({});