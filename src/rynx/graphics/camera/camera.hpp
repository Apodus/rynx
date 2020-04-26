#pragma once

#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>

namespace rynx {
	class ray;
	class camera {
	public:
		camera();
		~camera();

		void setOrtho(float width, float height, float zNear, float zFar);
		void setProjection(float zNear, float zFar, float aspect);

		void setPosition(vec3<float> cameraPosition);
		void setDirection(vec3<float> cameraDirection);
		void setUpVector(vec3<float> upVector);
		void rebuild_view_matrix();

		vec3f position() const { return m_position; }
		vec3f direction() const { return m_direction; }

		vec3f local_left() const { return -m_local_left; }
		vec3f local_up() const { return m_local_up; }
		vec3f local_forward() const { return m_local_forward; }

		const rynx::matrix4& getView() const;
		const rynx::matrix4& getProjection() const;

		rynx::ray ray_cast(float x, float y) const;

	private:
		rynx::matrix4 view;
		rynx::matrix4 projection;

		vec3f m_local_left;
		vec3f m_local_up;
		vec3f m_local_forward;

		vec3f m_position;
		vec3f m_direction;
		vec3f m_up;
	};
}
