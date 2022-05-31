
#include <rynx/scheduler/deadlock_detector.hpp>
#include <rynx/scheduler/task_scheduler.hpp>
#include <iostream>
#include <atomic>
#include <thread>

void rynx::scheduler::dead_lock_detector::print_error_recovery() {
	std::cout << "Recovered -- Frame complete" << std::endl << std::endl;
}

struct rynx::scheduler::dead_lock_detector::data {
	std::atomic<bool> dead_lock_detector_keepalive = true;
	std::thread detector_thread;
};

rynx::scheduler::dead_lock_detector::dead_lock_detector(rynx::scheduler::task_scheduler* host) {
	m_data = new rynx::scheduler::dead_lock_detector::data;
	m_data->detector_thread = std::thread([this, host]() {
		uint64_t prev_tick = 994839589;
		bool detector_triggered = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		while (m_data->dead_lock_detector_keepalive) {
			uint64_t tickCounter = host->tick_counter();
			if (!detector_triggered) {
				if (prev_tick == tickCounter) {
					host->dump();
					rynx::profiling::write_profile_log();
					detector_triggered = true;
				}
			}
			else if (prev_tick != tickCounter) {
				detector_triggered = false;
				print_error_recovery();
			}

			prev_tick = tickCounter;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
		});
}

rynx::scheduler::dead_lock_detector::~dead_lock_detector() {
	m_data->dead_lock_detector_keepalive = false;
	m_data->detector_thread.join();
	delete m_data;
}