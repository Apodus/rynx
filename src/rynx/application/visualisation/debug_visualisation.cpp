
#include <rynx/application/visualisation/debug_visualisation.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>

void rynx::application::DebugVisualization::addDebugVisual(mesh* mesh, rynx::matrix4 model, floats4 color, float lifetime) {
	auto& obj = m_data[mesh];
	obj.colors.emplace_back(color);
	obj.matrices.emplace_back(model);
	obj.lifetimes.emplace_back(lifetime);
}

void rynx::application::DebugVisualization::prepare(rynx::scheduler::context*) {
	for (auto&& entry : m_data) {
		for (auto& lifetime : entry.second.lifetimes) {
			lifetime -= 0.016f; // TODO: get dt from context or setter.
		}
		for (size_t i = 0; i < entry.second.lifetimes.size(); ++i) {
			if (entry.second.lifetimes[i] < 0) {
				entry.second.lifetimes[i] = entry.second.lifetimes.back();
				entry.second.lifetimes.pop_back();

				entry.second.colors[i] = entry.second.colors.back();
				entry.second.colors.pop_back();

				entry.second.matrices[i] = entry.second.matrices.back();
				entry.second.matrices.pop_back();
			}
		}
	}
}

void rynx::application::DebugVisualization::execute() {
	for (auto&& entry : m_data) {
		m_meshRenderer->drawMeshInstanced(*entry.first, entry.second.matrices, entry.second.colors);
	}
}