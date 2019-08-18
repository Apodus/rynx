
#pragma once

#include <mutex>
#include <condition_variable>

class semaphore {
	std::mutex m_mutex;
	std::condition_variable m_conditionVariable;
	int counter = 0;

public:
	semaphore() {}

	void signal() {
		std::unique_lock<std::mutex> lock(m_mutex);
		++counter;
		m_conditionVariable.notify_one();
	}

	void wait() {
		std::unique_lock<std::mutex> lock(m_mutex);
		while (!counter)
			m_conditionVariable.wait(lock);
		--counter;
	}
};