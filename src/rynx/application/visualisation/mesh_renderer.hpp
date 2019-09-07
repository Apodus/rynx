#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/application/render.hpp>
#include <rynx/application/components.hpp>

// required for implementation
#include <rynx/tech/ecs.hpp>

namespace rynx {
	namespace application {
		namespace visualisation {
			struct mesh_renderer : public rynx::application::renderer::irenderer {
				mesh_renderer(MeshRenderer* meshRenderer) {
					m_meshRenderer = meshRenderer;
				}
				virtual ~mesh_renderer() {}
				virtual void render(const rynx::ecs& ecs) override {
					rynx_profile(Game, "mesh_renderer");
					ecs.for_each([this](const rynx::components::position& pos, const rynx::components::mesh& m, const rynx::components::texture& t, const rynx::components::color& color) {
						matrix4 model;
						model.discardSetTranslate(pos.value);
						model.rotate(pos.angle, 0, 0, 1);
						m_meshRenderer->drawMesh(*m.m, model, t.tex, color.value);
					});
				}

				MeshRenderer* m_meshRenderer;
			};
		}
	}
}
