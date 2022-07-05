
#include <rynx/thread/fiber.hpp>
#include <rynx/tech/std/memory.hpp>

void* Fiber::getFiberAddress() {
	return m_fiber;
}

void Fiber::go(rynx::function<void(Fiber*)> func) {
	m_func = func;
	resume();
}

void Fiber::resume() {
#if defined(_WIN32)
	m_callingFiber = Fiber::getCurrentPlatformFiber();
#endif
	Fiber::switchToFiber(m_callingFiber, m_fiber);
}

void Fiber::yield() const {
	Fiber::switchToFiber(m_fiber, m_callingFiber);
}
