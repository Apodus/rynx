

#include <atomic>
#include <thread>

#include <rynx/tech/profiling.hpp>

namespace rynx {

	namespace scheduler {
		struct dead_lock_detector {
			std::atomic<bool> dead_lock_detector_keepalive = true;
			std::thread detector_thread;

			~dead_lock_detector() {
				dead_lock_detector_keepalive = false;
				detector_thread.join();
			}

			template<typename T>
			dead_lock_detector(T* host) {
				detector_thread = std::thread([this, host]() {
					uint64_t prev_tick = 994839589;
					bool detector_triggered = false;
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					while (dead_lock_detector_keepalive) {
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
							std::cout << "Recovered -- Frame complete" << std::endl << std::endl;
						}

						prev_tick = tickCounter;
						std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					}
				});
			}
		};
	}
}
