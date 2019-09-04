
#ifdef _WIN32

#include <rynx/thread/fiber.hpp>
#include <windows.h>

Fiber::Fiber(void* platformFiber) {
	m_isThread = true;
	m_fiber = platformFiber;
}

Fiber::Fiber() {
	m_isThread = false;
	m_fiber = CreateFiber(0, Fiber::fiberStartingFunction, this);
}

Fiber::~Fiber() {
	if(!m_isThread) {
		DeleteFiber(m_fiber);
	}
	m_fiber = nullptr;
}

void Fiber::switchToFiber(void* fromPtr, void* toPtr) {
	(void)fromPtr;
	SwitchToFiber(toPtr);
}

Fiber* Fiber::enterFiberMode() {
	return new Fiber(ConvertThreadToFiber(0));
}

void Fiber::exitFiberMode(Fiber* mainFiber) {
	if(!ConvertFiberToThread()) {
		// todo
	}

	delete mainFiber;
}

void* Fiber::getCurrentPlatformFiber() {
	return GetCurrentFiber();
}

void Fiber::fiberStartingFunction(void* data) {
	Fiber* fiber = reinterpret_cast<Fiber*>(data);
	while(true) {
		fiber->m_func(fiber);
		fiber->yield();
	}
}

#endif