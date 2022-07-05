
#include <rynx/application/visualisation/geometry/scrolling_background_2d.hpp>
#include <rynx/math/matrix.hpp>
#include <rynx/graphics/mesh/mesh.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>

rynx::application::visualization::scrolling_background_2d::scrolling_background_2d(rynx::graphics::renderer& mesh_renderer, rynx::shared_ptr<rynx::camera> camera, rynx::graphics::mesh* m) : m_bg_mesh(m), m_camera(camera), m_mesh_renderer(mesh_renderer) {}

void rynx::application::visualization::scrolling_background_2d::execute() {
	rynx::matrix4 model;
	model.discardSetTranslate(m_pos);
	
	
	model.scale(m_scale * m_aspect_ratio, m_scale * m_aspect_ratio, 1.0f);

	rynx::floats4 color{ 1, 1, 1, 1 };
	m_bg_mesh->bind();
	m_bg_mesh->rebuildTextureBuffer();
	m_mesh_renderer.drawMeshInstancedDeferred(*m_bg_mesh, 1, &model, &color, &m_bg_texture);
}

void rynx::application::visualization::scrolling_background_2d::prepare(rynx::scheduler::context*) {
	auto new_bg_pos = m_camera->position();
	new_bg_pos.z = 0.0f;
	m_scale = (m_camera->position().z - new_bg_pos.z);
	m_pos = new_bg_pos;

	auto& uvs = m_bg_mesh->texCoords;
	rynx_assert(uvs.size() == 8, "size mismatch");

	auto cam_pos = m_camera->position();

	float uv_x_mul = horizontal_motion_scale / (1024.0f * 0.325f);
	float uv_y_mul = vertical_motion_scale / (1024.0f * 0.325f);
	float uv_z_mul = 0.001f;
	int uv_i = 0;

	float uv_x_modifier = cam_pos.y * uv_x_mul + cam_pos.z * uv_z_mul;
	float uv_y_modifier = cam_pos.x * uv_y_mul + cam_pos.z * uv_z_mul;
	constexpr float back_ground_image_scale = 0.25f * 0.5f;

	uvs[uv_i++] = (1.0f / back_ground_image_scale + uv_x_modifier) * m_aspect_ratio;
	uvs[uv_i++] = (0.0f / back_ground_image_scale + uv_y_modifier) * m_aspect_ratio;

	uvs[uv_i++] = (0.0f / back_ground_image_scale + uv_x_modifier) * m_aspect_ratio;
	uvs[uv_i++] = (0.0f / back_ground_image_scale + uv_y_modifier) * m_aspect_ratio;

	uvs[uv_i++] = (0.0f / back_ground_image_scale + uv_x_modifier) * m_aspect_ratio;
	uvs[uv_i++] = (1.0f / back_ground_image_scale + uv_y_modifier) * m_aspect_ratio;

	uvs[uv_i++] = (1.0f / back_ground_image_scale + uv_x_modifier) * m_aspect_ratio;
	uvs[uv_i++] = (1.0f / back_ground_image_scale + uv_y_modifier) * m_aspect_ratio;
}

void rynx::application::visualization::scrolling_background_2d::set_aspect_ratio(float aspect_ratio) {
	if (aspect_ratio < 1.0f)
		m_aspect_ratio = 1.0f / aspect_ratio;
	else
		m_aspect_ratio = aspect_ratio;
}