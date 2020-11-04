
#include <rynx/input/mapped_input.hpp>
#include <rynx/math/geometry/ray.hpp>
#include <rynx/graphics/camera/camera.hpp>

rynx::ray rynx::mapped_input::mouseRay(rynx::camera& cam) {
	auto mousePos = userIO->getCursorPosition();
	return cam.ray_cast(mousePos.x, mousePos.y);
}