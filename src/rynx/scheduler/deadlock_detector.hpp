
#pragma once

#include <rynx/tech/profiling.hpp>

namespace rynx {

	namespace scheduler {
		class task_scheduler;
		
		struct dead_lock_detector {
			
			struct data;
			data* m_data = nullptr;

			dead_lock_detector(rynx::scheduler::task_scheduler* host);
			~dead_lock_detector();

			static void print_error_recovery();

		};
	}
}
