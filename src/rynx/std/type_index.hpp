
#pragma once

#include <cstdint>
#include <type_traits>

namespace rynx {
	namespace type_index {
		using type_id_t = uint64_t;
		
		struct used_type {
			used_type* next = nullptr;
			char const* name = nullptr;
			type_id_t* id = nullptr;
		};
		
		namespace internal {
			RynxStdDLL used_type* get_used_type_list();
			RynxStdDLL void set_used_type_list(used_type*);
		}
#ifndef _WIN64
		template<typename T> static char const* const unique_str_for_t() { return __PRETTY_FUNCTION__; }
#else
		template<typename T> static char const* const unique_str_for_t() { return __FUNCSIG__; }
#endif

		template<typename T> struct id_for_type {
			static type_id_t id;
		};

		template<typename T> type_id_t mark_type_is_used(id_for_type<T> t) {
			static used_type used_type_v { rynx::type_index::internal::get_used_type_list(), unique_str_for_t<T>(), &t.id};
			rynx::type_index::internal::set_used_type_list(&used_type_v);
			return ~type_id_t(0);
		}

		struct virtual_type {
			static constexpr type_id_t invalid = ~type_id_t(0);
			type_id_t type_value = invalid;
			bool operator == (type_id_t other) const noexcept { return other == type_value; }
		};

		RynxStdDLL void initialize();
		RynxStdDLL virtual_type create_virtual_type();
		RynxStdDLL const char* name_of_type(type_id_t id);

		template<typename T> type_id_t id() {
			using naked_t = std::remove_cv_t<std::remove_reference_t<T>>;
			return id_for_type<naked_t>::id;
		}
	};
}

template<typename T> uint64_t rynx::type_index::id_for_type<T>::id = rynx::type_index::mark_type_is_used<T>({});