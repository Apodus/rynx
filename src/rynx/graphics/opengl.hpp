#pragma once

#include <GL/glew.h>

#include <GL/gl.h>
#include <GL/glu.h>

#ifdef APIENTRY // winapi definition collides with glfw3 definition. they both define it as __stdcall
#undef APIENTRY
#endif

#include <GLFW/glfw3.h> // must be after glew and glu