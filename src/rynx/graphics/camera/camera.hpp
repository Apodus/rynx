#pragma once

#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/tech/smooth_value.hpp>

namespace rynx {
	class ray;
	class GraphicsDLL camera {
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

		camera& rotate(vec3f drot);
		camera& translate(vec3f dpos);
		camera& tick(float dt);

		camera& smooth_position_updates() { m_position_mode = value_mode::smooth; return *this; }
		camera& instant_position_updates() { m_position_mode = value_mode::instant; return *this; }
		camera& smooth_orientation_updates() { m_orientation_mode = value_mode::smooth; return *this; }
		camera& instant_orientation_updates() { m_orientation_mode = value_mode::instant; return *this; }

		camera& mode_free6() { m_mode = mode::free6; return *this; }
		camera& mode_fps() { m_mode = mode::fps; return *this; }

	private:
		enum class mode {
			free6,
			fps
		};

		enum class value_mode {
			instant,
			smooth
		};

		mode m_mode = mode::fps;
		value_mode m_position_mode = value_mode::smooth;
		value_mode m_orientation_mode = value_mode::smooth;

		rynx::matrix4 view;
		rynx::matrix4 projection;

		vec3f m_local_left;
		vec3f m_local_up;
		vec3f m_local_forward;

		rynx::smooth<vec3f> m_position;
		rynx::smooth<vec3f> m_direction;
		rynx::smooth<vec3f> m_up;
	};
}
