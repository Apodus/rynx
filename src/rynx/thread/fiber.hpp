
#pragma once

#include <functional>
#include <memory>

#ifndef _WIN32
#include <ucontext.h>
#endif

class Fiber {
private:
	Fiber(void* platformFiber);

public:
	Fiber();
	~Fiber();

	void go(std::function<void(Fiber*)>);
	void resume();
	void yield() const;

	static Fiber* enterFiberMode();
	static void exitFiberMode(Fiber*);

private:
	void* getCurrentPlatformFiber();
	void* getFiberAddress();
	
	static void switchToFiber(void* pFromFiber, void* pToFiber);

	std::function<void(Fiber*)> m_func;
	bool m_isThread;

#ifdef _WIN32
	static void __stdcall fiberStartingFunction(void*);
#else
public:
	static void fiberStartingFunction(void*);
private:
#endif

	void* m_callingFiber;
	void* m_fiber;

#ifndef _WIN32
	// assume linux
	ucontext_t caller;
	ucontext_t callee;
#endif
};
