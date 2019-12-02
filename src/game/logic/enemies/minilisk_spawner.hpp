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

			virtual void onFrameProcess(rynx::scheduler::context& scheduler, float /* dt */) override {
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
									rynx::components::radius(2.0f),
									rynx::components::collision_category(dynamic),
									rynx::components::color(),
									rynx::components::dampening({ 0.97f, 0.997f }),
									rynx::components::frame_collisions()
								);

								for (int k = 1; k < 40; ++k) {
									auto id2 = ecs.create(
										rynx::components::position({ x + 3.5f * k, y + 3.5f * k, 0 }),
										rynx::components::motion(),
										rynx::components::physical_body(5.0f, 15.0f, 0.3f, 1.0f),
										rynx::components::radius(2.0f),
										rynx::components::collision_category(dynamic),
										rynx::components::color(),
										rynx::components::dampening({ 0.80f, 0.90f }),
										rynx::components::frame_collisions()
									);

									rynx::components::rope joint;
									joint.id_a = id1;
									joint.id_b = id2;
									joint.point_a = vec3<float>(0, 0, 0);
									joint.point_b = vec3<float>(0, 0, 0);
									joint.length = 4.1f;
									joint.strength = 25.0f;
									ecs.create(joint);

									id1 = id2;
								}
							}
							else {
								float edge_length = 10.5f + 15.0f * (m_random() & 127) / 127.0f;
								float mass = 5000.0f;
								auto rectangle_moment_of_inertia = [](float mass, float width, float height) { return mass / 12.0f * (height * height + width * width); };
								ecs.create(
									rynx::components::position(vec3<float>(x, y, 0.0f), i * 2.0f),
									rynx::components::collision_category(dynamic),
									rynx::components::boundary({ Shape::makeBox(edge_length).generateBoundary_Outside() }),
									rynx::components::radius(math::sqrt_approx(2 * edge_length * edge_length)),
									rynx::components::physical_body(mass, rectangle_moment_of_inertia(mass, edge_length, edge_length), 0.2f, 1.0f),
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
