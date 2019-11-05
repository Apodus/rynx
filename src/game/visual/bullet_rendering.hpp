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
			bullet_renderer(rynx::MeshRenderer* meshRenderer) {
				m_meshRenderer = meshRenderer;
			}
			virtual ~bullet_renderer() {}
			virtual void render(const rynx::ecs& ecs) override {
				rynx_profile("visualisation", "bullets");
				ecs.for_each([this](const rynx::components::position& pos, const rynx::components::motion& m, const game::components::bullet&, const rynx::components::color& color) {
					m_meshRenderer->drawLine(pos.value - m.velocity, pos.value, 0.3f, color.value);
				});
			}

			rynx::MeshRenderer* m_meshRenderer;
		};
	}
}

namespace game {
	namespace visual {
		struct hero_renderer : public rynx::application::renderer::irenderer {
			hero_renderer(rynx::MeshRenderer* meshRenderer) {
				m_meshRenderer = meshRenderer;
			}
			virtual ~hero_renderer() {}
			virtual void render(const rynx::ecs& ecs) override {
				/*
				rynx_profile("visualisation", "hero renderer");
				ecs.query().execute([this](
					const rynx::components::position& pos,
					const rynx::components::radius& r,
					const rynx::components::color& color)
					{
						matrix4 model;
						model.discardSetTranslate(pos.value);
						model.scale(r.r);
							
						// m_meshRenderer->drawRectangle(model, "Helmet", color.value);
							
						// TODO: pos.angle != aim direction.
						model.discardSetTranslate(pos.value);
						model.scale(r.r * 2, r.r, 1.0f);
						model.rotate(pos.angle, 0, 0, 1);
						// m_meshRenderer->drawRectangle(model, "Shoulders", color.value);

						// TODO: Legs while running :)
					});
				*/
			}

			rynx::MeshRenderer* m_meshRenderer;
		};
	}
}
