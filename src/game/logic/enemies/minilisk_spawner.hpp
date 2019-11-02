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
			uint64_t how_often_to_spawn = 300;
			float x_spawn = -20.9f;
			
			math::rand64 m_random;

			minilisk_test_spawner_logic(rynx::collision_detection::category_id dynamic) : dynamic(dynamic) {}

			virtual void onFrameProcess(rynx::scheduler::context& scheduler) override {
				if ((++frameCount % how_often_to_spawn) == 0) {
					scheduler.add_task("create monsters", [this](
						rynx::ecs::edit_view<
							game::components::minilisk,
							game::health,
							rynx::components::position,
							rynx::components::motion,
							rynx::components::physical_body,
							rynx::components::radius,
							rynx::components::collision_category,
							rynx::components::color,
							rynx::components::dampening,
							rynx::components::boundary,
							rynx::components::rope,
							rynx::components::frame_collisions> ecs) {
						for (int i = 0; i < 1; ++i) {
							float x = x_spawn + m_random(0.0f, 4.0f);
							float y = +100.0f + m_random(0.0f, 4.0f);
							if (true || m_random() & 1) {
								auto id1 = ecs.create(
									rynx::components::position({x, y, 0 }),
									rynx::components::motion(),
									rynx::components::physical_body(5.0f, 15.0f, 0.3f, 1.0f),
									rynx::components::radius(1.6f),
									rynx::components::collision_category(dynamic),
									rynx::components::color(),
									rynx::components::dampening({ 0.97f, 0.997f }),
									rynx::components::frame_collisions()
								);

								for (int k = 1; k < 40; ++k) {
									auto id2 = ecs.create(
										rynx::components::position({ x + 1 * k, y + 1 * k, 0 }),
										rynx::components::motion(),
										rynx::components::physical_body(5.0f, 15.0f, 0.3f, 1.0f),
										rynx::components::radius(1.0f),
										rynx::components::collision_category(dynamic),
										rynx::components::color(),
										rynx::components::dampening({ 0.97f, 0.997f }),
										rynx::components::frame_collisions()
									);

									rynx::components::rope joint;
									joint.id_a = id1;
									joint.id_b = id2;
									joint.point_a = vec3<float>(0, 0, 0);
									joint.point_b = vec3<float>(0, 0, 0);
									joint.length = 0.6f;
									joint.strength = 10.0f;
									ecs.create(joint);

									id1 = id2;
								}
							}
							else {
								ecs.create(
									rynx::components::position(vec3<float>(x, y, 0.0f), i * 2.0f),
									rynx::components::collision_category(dynamic),
									rynx::components::boundary({ Shape::makeBox(1.5f + 2.0f * (m_random() & 127) / 127.0f).generateBoundary_Outside() }),
									rynx::components::radius(math::sqrt_approx(16 + 16)),
									rynx::components::physical_body(5.0f, 15.0f, 0.3f, 1.0f),
									rynx::components::color({ 1,1,0,1 }),
									rynx::components::motion({ 0, 0, 0 }, 0),
									rynx::components::dampening({ 0.97f, 0.997f }),
									rynx::components::frame_collisions()
								);
							}
						}
					});
				}
			}
		};
	}
}
