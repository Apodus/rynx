#include "camera.hpp"

Camera::Camera() {

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

void Camera::setPosition(const vec3<float>& cameraPosition_) {
	cameraPosition = cameraPosition_;
	vec3<float> center(cameraPosition);
	vec3<float> up(0.f, 1.f, 0.f);
	center.z -= 10;

	view.discardSetLookAt(
		cameraPosition.x, cameraPosition.y, cameraPosition.z, 
		center.x, center.y, center.z, 
		up.x, up.y, up.z
	);
}

const matrix4& Camera::getView() const {
	return view;
}

const matrix4& Camera::getProjection() const {
	return projection;
}