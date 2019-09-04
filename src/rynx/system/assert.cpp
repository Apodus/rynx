
#include <rynx/system/assert.hpp>

#ifdef _WIN32
#include "Windows.h"

void windowsDebugOut(const char* logBuffer) {
	OutputDebugStringA(logBuffer);
}
#endif