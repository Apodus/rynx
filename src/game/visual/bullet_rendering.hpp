#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/application/render.hpp>
#include <rynx/application/components.hpp>
#include <rynx/tech/ecs.hpp>
#include <game/logic/shooting.hpp>

namespace game {
	namespace visual {
		struct bullet_renderer : public rynx::application::renderer::irenderer {
			bullet_renderer(MeshRenderer* meshRenderer) {
				m_meshRenderer = meshRenderer;
			}
			virtual ~bullet_renderer() {}
			virtual void render(const rynx::ecs& ecs) override {
				ecs.for_each([this](const rynx::components::position& pos, const rynx::components::motion& m, const game::components::bullet&, const rynx::components::color& color) {
					m_meshRenderer->drawLine(pos.value - m.velocity, pos.value, 0.1f, color.value);
				});
			}

			MeshRenderer* m_meshRenderer;
		};
	}
}