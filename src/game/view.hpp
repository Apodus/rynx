#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/graphics/renderer/textrenderer.hpp>
#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/camera/camera.hpp>

#include <rynx/graphics/text/fontdata/lenka.hpp>
#include <rynx/graphics/text/fontdata/consolamono.hpp>
#include <rynx/input/userio.hpp>
#include <rynx/application/visualisation/renderer.hpp>

#include <rynx/application/components.hpp>

#include <game/logic.hpp>
#include <game/gametypes.hpp>

#include <rynx/tech/profiling.hpp>

namespace game
{
	struct hitpoint_bar_renderer : public rynx::application::graphics_step::igraphics_step {
		hitpoint_bar_renderer(rynx::MeshRenderer* meshRenderer) {
			m_meshRenderer = meshRenderer;
		}
		virtual ~hitpoint_bar_renderer() {}
		
		virtual void prepare(rynx::scheduler::context* ctx) override {}

		virtual void execute() override {
			/*
			rynx_profile("SphereTree", "DrawHitpoints");
			ecs.for_each([this](const rynx::components::position& pos, const health& hp) {
				auto drawPos = pos.value + vec3<float>({ -0.5f, 1.0f, 0.0f });
				float prctage = hp.currentHp / hp.maxHp;
				auto drawPosTo = drawPos + vec3<float>({ prctage, 0.0f, 0.0f });
				m_meshRenderer->drawLine(drawPos, drawPosTo, 0.2f, floats4(1.0f, 0.0f, 0.0f, 1.0f));
			});
			*/
		}

		rynx::MeshRenderer* m_meshRenderer;
	};
}
