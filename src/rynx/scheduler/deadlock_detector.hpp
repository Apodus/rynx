
#pragma once

#include <rynx/profiling/profiling.hpp>

namespace rynx {

	namespace scheduler {
		class task_scheduler;
		
		struct SchedulerDLL dead_lock_detector {
			
			struct data;
			data* m_data = nullptr;

			dead_lock_detector(rynx::scheduler::task_scheduler* host);
			~dead_lock_detector();

			static void print_error_recovery();

		};
	}
}
