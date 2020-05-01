#pragma once

#include <memory>
#include <functional>

namespace rynx {
	template<typename T> using opaque_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;
}