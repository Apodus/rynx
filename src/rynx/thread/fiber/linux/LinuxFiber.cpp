
#if __linux__

#include <ucontext.h>
#include <cstdint>

#include <rynx/thread/fiber.hpp>

// TLS pointer, since multiple threads can be in fiber mode.
thread_local ucontext_t g_activeContext;
thread_local ucontext_t g_prevContext;

void linuxFiberStart(unsigned a, unsigned b) {
	Fiber* ptr = reinterpret_cast<Fiber*>((static_cast<uint64_t>(a) << 32) | static_cast<uint64_t>(b));
	Fiber::fiberStartingFunction(ptr);
}

Fiber::Fiber(void* platformFiber) {
	m_fiber = platformFiber;
	m_isThread = true;
	m_callingFiber = nullptr;
}

Fiber::Fiber() {
	getcontext(&callee);
	callee.uc_stack.ss_size = 1024 * 1024;
	callee.uc_stack.ss_sp = new char[1024 * 1024]; // 1MB stack
	
	unsigned highBits = (reinterpret_cast<long long>(this) >> 32);
	unsigned lowBits = reinterpret_cast<long long>(this);

	makecontext(&callee, reinterpret_cast<void (*)()>(&linuxFiberStart), 2, highBits, lowBits);
	m_fiber = &callee;
	m_callingFiber = &caller; // this can be left uninitialized, swapcontext does the job.
	m_isThread = false;
}

Fiber::~Fiber() {
	if(m_isThread) {
	}
	else {
		delete[] (char*)callee.uc_stack.ss_sp; // release stack
	}
	m_fiber = nullptr;
}

void Fiber::switchToFiber(void* fromPtr, void* toPtr) {
	swapcontext(reinterpret_cast<ucontext_t*>(fromPtr), reinterpret_cast<ucontext_t*>(toPtr));
}

Fiber* Fiber::enterFiberMode() {
	return new Fiber(reinterpret_cast<void*>(&g_activeContext));
}

void Fiber::exitFiberMode(Fiber* mainFiber) {
	delete mainFiber;
}

void* Fiber::getCurrentPlatformFiber() {
	return &g_activeContext; // NOTE: this hack breaks down if trying to use a fiber from inside a fiber.
}

void Fiber::fiberStartingFunction(void* data) {
	Fiber* fiber = reinterpret_cast<Fiber*>(data);
	while(true) {
		fiber->m_func(fiber);
		fiber->yield();
	}
}

#endif
