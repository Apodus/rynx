
#pragma once

#include <rynx/std/string.hpp>

namespace rynx {
	namespace profiling {
		constexpr bool enabled = true;

		ProfilingDLL void push_event_begin(
			rynx::string name,
			rynx::string category,
			uint64_t pid
		);

		ProfilingDLL void push_event_end(
			uint64_t pid
		);

		ProfilingDLL void write_profile_log();

		struct scope {
			scope(rynx::string name, rynx::string category) {
				// TODO: profiling should ensure that names are not dynamically allocated.
				// rynx_assert(rynx::is_small_space(name), "profiling string requires dynamic alloc, please make it shorter; '%s'", name.c_str());
				// rynx_assert(rynx::is_small_space(category), "profiling string requires dynamic alloc, please make it shorter; '%s'", category.c_str());
				if constexpr (enabled) {
					push_event_begin(
						name,
						category,
						0
					);
				}
			}

			~scope() {
				if constexpr (enabled) {
					push_event_end(0);
				}
			}
		};
	}
}

#if 1
#define RYNX_MACRO_HELPER_CONCAT(a, b) a##b
#define RYNX_MACRO_HELPER_CONCAT_EXPAND(a, b) RYNX_MACRO_HELPER_CONCAT(a, b)
#define rynx_profile(a, b) rynx::profiling::scope RYNX_MACRO_HELPER_CONCAT_EXPAND(RYNX_MACRO_HELPER_CONCAT_EXPAND(profile_scope, __COUNTER__), __LINE__)(b, a);
#else
#define rynx_profile(a, b)
#endif