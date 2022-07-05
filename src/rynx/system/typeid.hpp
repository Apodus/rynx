#pragma once

#include <rynx/tech/std/string.hpp>

namespace rynx {
	namespace traits {
		template<typename T>
		rynx::string type_name() {
			return typeid(T).name();
		}
	}
}