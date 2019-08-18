
#include <rynx/system/assert.hpp>
#include "Windows.h"

void windowsDebugOut(const char* logBuffer) {
	OutputDebugStringA(logBuffer);
}