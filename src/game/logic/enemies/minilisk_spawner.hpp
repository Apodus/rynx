#pragma once

#include <rynx/application/components.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/tech/ecs.hpp>
#include <vector>

#include <rynx/tech/math/vector.hpp>
#include <rynx/graphics/mesh/math.hpp>

#include <game/gametypes.hpp>

namespace game {
	namespace logic {
		struct minilisk_test_spawner_logic : public rynx::application::logic::iruleset {
			rynx::collision_detection::category_id dynamic;
			uint64_t frameCount = 0;

			minilisk_test_spawner_logic(rynx::collision_detection::category_id dynamic) : dynamic(dynamic) {}

			virtual void onFrameProcess(rynx::scheduler::context& scheduler) override {
				if ((++frameCount & 63) > 60) {
					scheduler.add_task("create monsters", [this](
						rynx::ecs::edit_view<
							game::components::minilisk,
							game::health,
							rynx::components::position,
							rynx::components::motion,
							rynx::components::mass,
							rynx::components::radius,
							rynx::components::collision_category,
							rynx::components::color,
							rynx::components::dampening,
							rynx::components::frame_collisions> ecs) {
						ecs.create(
							game::components::minilisk(),
							game::health({ 30, 30 }),
							rynx::components::position({ -20.0f + float(rand()) / RAND_MAX, +20.0f + float(rand()) / RAND_MAX, 0 }),
							rynx::components::motion(),
							rynx::components::mass({ 0.2f }),
							rynx::components::radius(0.3f),
							rynx::components::collision_category(dynamic),
							rynx::components::color(),
							rynx::components::dampening({ 0.87f, 0.87f }),
							rynx::components::frame_collisions()
						);
					});
				}
			}
		};
	}
}
