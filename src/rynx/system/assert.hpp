#pragma once

#include <cstdio>

#ifdef RYNX_ASSERTS_ENABLED_

#define DEBUG_LEVEL 2

#ifdef _WIN32
void SystemDLL windowsDebugOut(const char* logBuffer);
#ifdef APIENTRY // glfw definition collides with winapi definition. they both define it as __stdcall
#undef APIENTRY
#endif
#endif

#define LOG_ACTUAL_DONT_USE__(x, ...) printf("%s::%s (%d): " x "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOG_SIMPLE_ACTUAL_DONT_USE__(x, ...) printf(x, ##__VA_ARGS__)
#define LOG_SIMPLE_NEWLINE_ACTUAL_DONT_USE__(x, ...) printf(x "\n", ##__VA_ARGS__)

#ifdef _WIN32
#define rynx_assert(x, msg, ...) {if(!(##x)){LOG_ACTUAL_DONT_USE__("ASSERT(%s) failed: " msg, #x, ##__VA_ARGS__); char assertBuffer[1024]; sprintf_s(assertBuffer, 1024,  "%s(%d): " #x "\n" ##msg "\n", __FILE__, __LINE__, ##__VA_ARGS__); windowsDebugOut(assertBuffer); __debugbreak(); } }
#else
#define rynx_assert(x, msg, ...) {if(!(x)){LOG_ACTUAL_DONT_USE__("ASSERT(%s) failed: " msg, #x, ##__VA_ARGS__); __builtin_trap(); } }
#endif

#if DEBUG_LEVEL > 1
#ifdef _WIN32

#define logmsg(x, ...) {                                 \
	/* printf(x "\n", ##__VA_ARGS__); */         \
	char logBuffer[2048];                             \
	sprintf_s(logBuffer, sizeof(logBuffer),  "%s(%d): " x "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
	windowsDebugOut(logBuffer); \
}

// log simple, no newline
#define logs_nn(x, ...) {                                 \
	LOG_SIMPLE_ACTUAL_DONT_USE__(x, ##__VA_ARGS__);          \
	char logBuffer[2048];                             \
	sprintf_s(logBuffer, sizeof(logBuffer),  x, ##__VA_ARGS__); \
	windowsDebugOut(logBuffer); \
}

// log simple (discards file & line info)
#define logs(x, ...) {                                 \
	LOG_SIMPLE_NEWLINE_ACTUAL_DONT_USE__(x, ##__VA_ARGS__);          \
	char logBuffer[2048];                             \
	sprintf_s(logBuffer, sizeof(logBuffer),  x "\n", ##__VA_ARGS__); \
	windowsDebugOut(logBuffer); \
}

#define logmsg_once_per_milliseconds(interval_ms, x, ...) { \
	/* printf(x "\n", ##__VA_ARGS__); */         \
	static std::chrono::time_point<std::chrono::high_resolution_clock> last_triggered; \
	auto timepoint = std::chrono::high_resolution_clock::now(); \
	if(std::chrono::duration_cast<std::chrono::milliseconds>(timepoint - last_triggered).count() > interval_ms) { \
		last_triggered = timepoint; \
		char logBuffer[2048];                             \
		sprintf_s(logBuffer, sizeof(logBuffer),  "%s(%d): " x "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
		windowsDebugOut(logBuffer); \
	} \
}

#else
#define logmsg(x, ...) LOG_ACTUAL_DONT_USE__(x, ##__VA_ARGS__)
#define logs(x, ...) LOG_SIMPLE_ACTUAL_DONT_USE__(x, ##__VA_ARGS__)
#define logsnl(x, ...) LOG_SIMPLE_NEWLINE_ACTUAL_DONT_USE__(x, ##__VA_ARGS__)
#define logmsg_once_per_milliseconds(interval_ms, x, ...) { \
	/* printf(x "\n", ##__VA_ARGS__); */         \
	static std::chrono::time_point<std::chrono::high_resolution_clock> last_triggered; \
	auto timepoint = std::chrono::high_resolution_clock::now(); \
	if(std::chrono::duration_cast<std::chrono::milliseconds>(timepoint - last_triggered).count() > interval_ms) { \
		last_triggered = timepoint; \
		LOG_ACTUAL_DONT_USE__(x, ##__VA_ARGS__) \
	} \
}
#endif
#else
#define logmsg(x, ...)
#define logs(x, ...)
#define logsnl(x, ...)
#define logmsg_once_per_milliseconds(...)
#endif

#else // no debug

#define logmsg(x, ...) 
#ifdef _WIN32
#define rynx_assert(x, msg, ...) __assume(x);
#else
#define rynx_assert(x, msg, ...) __builtin_assume(x);
#endif
#define logs(x, ...)
#define logsnl(x, ...)
#define logmsg_once_per_milliseconds(...)

#endif
