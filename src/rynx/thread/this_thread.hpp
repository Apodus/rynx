
#pragma once

#include <cinttypes>

namespace rynx {
	namespace this_thread {
		int64_t id();

		struct rynx_thread_raii {
			rynx_thread_raii();
			~rynx_thread_raii();
		};
	}
}
