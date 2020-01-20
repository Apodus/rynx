#include "camera.hpp"

Camera::Camera() {
	view.identity();
	projection.identity();

	m_position = vec3f(0, 0, 0);
	m_direction = vec3f(0, 0, -1);
	m_up = vec3f(0, 1, 0);
}

Camera::~Camera() {

}

void Camera::setProjection(float zNear, float zFar, float aspect) {
	float w = zNear;
	float h = zNear / aspect;
	projection.discardSetFrustum(
		-w, w,
		-h, h,
		zNear, zFar
	);
}

void Camera::setOrtho(float width, float height, float zNear, float zFar)
{
	projection.discardSetOrtho(
		-width * 0.5f, +width * 0.5f,
		-height * 0.5f, +height * 0.5f,
		zNear, zFar
	);
}

void Camera::setPosition(vec3<float> cameraPosition) {
	m_position = cameraPosition;
}

void Camera::setDirection(vec3<float> cameraDirection) {
	m_direction = cameraDirection;
}

void Camera::setUpVector(vec3<float> upVector) {
	m_up = upVector;
}

void Camera::rebuild_view_matrix() {
	m_local_left = m_direction.normalize().cross(m_up).normalize();
	m_local_up = m_local_left.cross(m_direction).normalize();
	m_local_forward = m_direction;

	view.discardSetLookAt(
		m_position,
		m_direction,
		m_up
	);
}

const rynx::matrix4& Camera::getView() const {
	return view;
}

const rynx::matrix4& Camera::getProjection() const {
	return projection;
}