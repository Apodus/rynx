#include <rynx/graphics/camera/camera.hpp>
#include <rynx/math/geometry/ray.hpp>

rynx::camera::camera() {
	view.identity();
	projection.identity();

	m_position = vec3f(0, 0, 0);
	m_direction = vec3f(0, 0, -1);
	m_up = vec3f(0, 1, 0);
}

rynx::camera::~camera() {

}

void rynx::camera::setProjection(float zNear, float zFar, float aspect) {
	float w = zNear;
	float h = zNear / aspect;
	projection.discardSetFrustum(
		-w, w,
		-h, h,
		zNear, zFar
	);
}

void rynx::camera::setOrtho(float width, float height, float zNear, float zFar)
{
	projection.discardSetOrtho(
		-width * 0.5f, +width * 0.5f,
		-height * 0.5f, +height * 0.5f,
		zNear, zFar
	);
}

void rynx::camera::setPosition(vec3<float> cameraPosition) {
	m_position = cameraPosition;
}

void rynx::camera::setDirection(vec3<float> cameraDirection) {
	m_direction = cameraDirection;
}

void rynx::camera::setUpVector(vec3<float> upVector) {
	m_up = upVector;
}

void rynx::camera::rebuild_view_matrix() {
	m_local_left = m_direction.normalize().cross(m_up).normalize();
	m_local_up = m_local_left.cross(m_direction).normalize();
	m_local_forward = m_direction;

	view.discardSetLookAt(
		m_position,
		m_direction,
		m_up
	);
}

const rynx::matrix4& rynx::camera::getView() const {
	return view;
}

const rynx::matrix4& rynx::camera::getProjection() const {
	return projection;
}

rynx::ray rynx::camera::ray_cast(float x, float y) const {
	vec4f ray_eye = matrix4(projection).invert() * vec4f(x, y, -1, +1);
	ray_eye.z = -1;
	ray_eye.w = +0;

	vec3f ray_world = (view * ray_eye).xyz();
	ray_world.normalize();
	return rynx::ray(m_position, ray_world);
}
