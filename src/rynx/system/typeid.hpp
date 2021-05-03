#pragma once

#include <string>

namespace rynx {
	namespace traits {
		template<typename T>
		std::string type_name() {
			return typeid(T).name();
		}
	}
}