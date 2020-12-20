#include <rynx/graphics/camera/camera.hpp>
#include <rynx/math/geometry/ray.hpp>

rynx::camera::camera() {
	view.identity();
	projection.identity();

	m_position = vec3f(0, 0, 0);
	m_direction = vec3f(0, 0, -1);
	m_up = vec3f(0, 1, 0);
}

rynx::camera::~camera() {}

void rynx::camera::setProjection(float zNear, float zFar, float aspect) {
	float w = zNear;
	float h = zNear / aspect;
	projection.discardSetFrustum(
		-w, w,
		-h, h,
		zNear, zFar
	);
}

void rynx::camera::setOrtho(float width, float height, float zNear, float zFar) {
	projection.discardSetOrtho(
		-width * 0.5f, +width * 0.5f,
		-height * 0.5f, +height * 0.5f,
		zNear, zFar
	);
}

void rynx::camera::setPosition(vec3<float> cameraPosition) {
	m_position = cameraPosition;
	if (m_position_mode == value_mode::instant) {
		m_position.tick(1.0f);
	}
}

void rynx::camera::setDirection(vec3<float> cameraDirection) {
	m_direction = cameraDirection;
	if (m_orientation_mode == value_mode::instant) {
		m_direction.tick(1.0f);
	}
}

void rynx::camera::setUpVector(vec3<float> upVector) {
	m_up = upVector;
	if (m_orientation_mode == value_mode::instant) {
		m_up.tick(1.0f);
	}
}

void rynx::camera::rebuild_view_matrix() {
	m_local_left = m_direction->normalize().cross(m_up).normalize();
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

rynx::camera& rynx::camera::rotate(rynx::vec3f drot) {
	if (m_mode == mode::fps) {
		rynx::matrix4 rotate_x;
		rynx::matrix4 rotate_y;
		rotate_x.discardSetRotation(drot.x, { 0, 1, 0 });
		rotate_y.discardSetRotation(drot.y, local_left());
		
		m_direction = rotate_y * rotate_x * m_direction.target_value();
		if (m_orientation_mode == value_mode::instant) {
			m_direction.tick(1.0f);
		}
	}
	else if (m_mode == mode::free6) {
		rynx::matrix4 rotor_x;
		rynx::matrix4 rotor_y;
		rynx::matrix4 rotor_z;
		
		rotor_x.discardSetRotation(drot.x, local_up());
		rotor_y.discardSetRotation(drot.y, local_left());
		rotor_z.discardSetRotation(drot.z, local_forward());
		
		rynx::matrix4 rotor = rotor_y * rotor_x * rotor_z;
		m_direction = rotor * m_direction.target_value();
		m_up = rotor * m_up.target_value();

		if (m_orientation_mode == value_mode::instant) {
			m_direction.tick(1.0f);
			m_up.tick(1.0f);
		}
	}
	return *this;
}

rynx::camera& rynx::camera::translate(rynx::vec3f dpos) {
	m_position += dpos;
	if (m_position_mode == value_mode::instant) {
		m_position.tick(1.0f);
	}
	return *this;
}

rynx::camera& rynx::camera::tick(float dt) {
	m_position.tick(dt);
	m_direction.tick(dt);
	m_up.tick(dt);
	return *this;
}