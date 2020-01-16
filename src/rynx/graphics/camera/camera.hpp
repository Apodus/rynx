#pragma once

#include <rynx/tech/math/matrix.hpp>
#include <rynx/tech/math/vector.hpp>

class Camera {
public:
	Camera();
	~Camera();

	void setOrtho(float width, float height, float zNear, float zFar);
	void setProjection(float zNear, float zFar, float aspect);
	void setPosition(vec3<float> cameraPosition);

	vec3<float> position() const { return cameraPosition; }

	const rynx::matrix4& getView() const;
	const rynx::matrix4& getProjection() const;
private:
	rynx::matrix4 view, projection;
	vec3<float> cameraPosition;
};