
#pragma once

#include <cinttypes>

namespace rynx {
	namespace this_thread {
		ThreadDLL int64_t id();

		struct ThreadDLL rynx_thread_raii {
			rynx_thread_raii();
			~rynx_thread_raii();
		};
	}
}
