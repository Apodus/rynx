
#pragma once

#include <rynx/tech/math/vector.hpp>

namespace Color {
	static constexpr vec4<float> WHITE(1.0f, 1.0f, 1.0f, 1.0f);
	static constexpr vec4<float> BLACK(0,0,0,1);
	static constexpr vec4<float> BLUE(0,0,1,1);
	static constexpr vec4<float> GREEN(0,1,0,1);
	static constexpr vec4<float> CYAN(0,1,1,1);
	static constexpr vec4<float> RED(1,0,0,1);
	static constexpr vec4<float> DARK_RED(0.5f,0,0,1);
	static constexpr vec4<float> GREY(0.5f, 0.5f, 0.5f, 1.0f);
	static constexpr vec4<float> MAGENTA(1,0,1,1);
	static constexpr vec4<float> YELLOW(1,1,0,1);
	static constexpr vec4<float> ORANGE(1, 0.5f, 0.25f, 1);
	static constexpr vec4<float> LIGHT_BLUE(0.5f, 1.0f, 1.0f, 1);
	static constexpr vec4<float> LIGHT_RED(1.0f, 0.5f, 0.3f, 1);
	static constexpr vec4<float> BROWN(0.5f, 0.2f, 0.1f, 1);
	static constexpr vec4<float> GOLDEN(249/256.0f, 229/256.0f, 0, 1);
	static constexpr vec4<float> DARK_GREY(0.3f, 0.3f, 0.3f, 1);
	static constexpr vec4<float> DARK_GREEN(0, 0.4f, 0, 1);
}