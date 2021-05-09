
#include <rynx/application/visualisation/debug_visualisation.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>


#include <rynx/math/spline.hpp>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/graphics/mesh/mesh.hpp>
#include <memory>

rynx::application::DebugVisualization::DebugVisualization(std::shared_ptr<rynx::graphics::renderer> meshRenderer) : m_meshRenderer(meshRenderer) {
	// define debug circle mesh
	{
		std::vector<rynx::vec3f> vertices = rynx::Shape::makeCircle(1.0f, 1024).as_vertex_vector();
		auto m = std::make_unique<rynx::graphics::mesh>();
		m->setModeLineStrip();

		short i = 0;
		for (auto v : vertices) {
			m->putVertex(v);
			m->putNormal(0, 1, 0);
			m->putUVCoord(0.5f, 0.5f);
			m->indices.emplace_back(i++);
		}

		m_circle_mesh = m_meshRenderer->meshes()->create_transient(std::move(m));
	}

	// define debug line mesh
	{
		auto m = std::make_unique<rynx::graphics::mesh>();
		m->setModeLineStrip();

		m->putVertex({-0.5f, 0.0f, 0.0f});
		m->putNormal(0, 1, 0);
		m->putUVCoord(0.5f, 0.5f);
		m->indices.emplace_back(static_cast<short>(0));

		m->putVertex({ +0.5f, 0.0f, 0.0f });
		m->putNormal(0, 1, 0);
		m->putUVCoord(0.5f, 0.5f);
		m->indices.emplace_back(static_cast<short>(1));

		m_line_mesh = m_meshRenderer->meshes()->create_transient(std::move(m));
	}
}

void rynx::application::DebugVisualization::addDebugLine(rynx::vec3f point_a, rynx::vec3f point_b, rynx::floats4 color, float lifetime) {
	auto* mesh = this->m_meshRenderer->meshes()->get(m_line_mesh);
	auto& obj = m_data[mesh];

	rynx::vec3f mid = (point_a + point_b) * 0.5f;
	rynx::vec3f delta = (point_a - point_b);
	rynx::matrix4 model;
	model.discardSetTranslate(mid);
	model.rotate_2d(rynx::math::atan_approx(delta.y / delta.x));
	model.scale(delta.length());

	obj.colors.emplace_back(color);
	obj.matrices.emplace_back(model);
	obj.lifetimes.emplace_back(lifetime);
	obj.textures.emplace_back(rynx::graphics::texture_id{});
}

void rynx::application::DebugVisualization::addDebugCircle(matrix4 model, floats4 color, float lifetime) {
	auto* mesh = this->m_meshRenderer->meshes()->get(m_circle_mesh);
	auto& obj = m_data[mesh];
	obj.colors.emplace_back(color);
	obj.matrices.emplace_back(model);
	obj.lifetimes.emplace_back(lifetime);
	obj.textures.emplace_back(rynx::graphics::texture_id{});
}

void rynx::application::DebugVisualization::addDebugVisual(rynx::graphics::mesh* mesh, rynx::matrix4 model, floats4 color, float lifetime) {
	auto& obj = m_data[mesh];
	obj.colors.emplace_back(color);
	obj.matrices.emplace_back(model);
	obj.lifetimes.emplace_back(lifetime);
	obj.textures.emplace_back(rynx::graphics::texture_id{});
}

void rynx::application::DebugVisualization::addDebugVisual(rynx::graphics::mesh_id mesh_id, rynx::matrix4 model, floats4 color, float lifetime) {
	addDebugVisual(m_meshRenderer->meshes()->get(mesh_id), model, color, lifetime);
}

void rynx::application::DebugVisualization::addDebugVisual(std::string meshName, rynx::matrix4 model, floats4 color, float lifetime) {
	addDebugVisual(m_meshRenderer->meshes()->findByName(meshName), model, color, lifetime);
}

void rynx::application::DebugVisualization::prepare(rynx::scheduler::context*) {
	for (auto&& entry : m_data) {
		for (size_t i = 0; i < entry.second.lifetimes.size(); ++i) {
			if (entry.second.lifetimes[i] < 0) {
				entry.second.lifetimes[i] = entry.second.lifetimes.back();
				entry.second.lifetimes.pop_back();

				entry.second.colors[i] = entry.second.colors.back();
				entry.second.colors.pop_back();

				entry.second.matrices[i] = entry.second.matrices.back();
				entry.second.matrices.pop_back();

				entry.second.textures[i] = entry.second.textures.back();
				entry.second.textures.pop_back();
			}
		}
		for (auto& lifetime : entry.second.lifetimes) {
			lifetime -= 0.016f; // TODO: get dt from context or setter.
		}
	}
}

void rynx::application::DebugVisualization::execute() {
	for (auto&& entry : m_data) {
		m_meshRenderer->drawMeshInstanced(*entry.first, entry.second.matrices, entry.second.colors, entry.second.textures);
	}
}