
#pragma once

#include <string>
#include <chrono>

namespace rynx {
	namespace profiling {
		constexpr bool enabled = true;

		void push_event_begin(
			std::string& name,
			std::string& category,
			uint64_t pid
		);

		void push_event_end(
			uint64_t pid
		);

		void write_profile_log();

		struct scope {
			scope(std::string name, std::string category) {
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


#define rynx_profile(a, b) rynx::profiling::scope abba(b, a);
