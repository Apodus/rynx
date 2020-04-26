#pragma once

#include <rynx/application/components.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/tech/ecs.hpp>
#include <vector>

#include <rynx/math/vector.hpp>
#include <rynx/math/geometry/math.hpp>

#include <game/gametypes.hpp>

namespace game {
	namespace logic {
		struct minilisk_test_spawner_logic : public rynx::application::logic::iruleset {
			std::shared_ptr<rynx::mesh_collection> m_meshes;
			rynx::collision_detection::category_id dynamic;
			uint64_t frameCount = 0;
			uint64_t how_often_to_spawn = 300;
			float x_spawn = -20.9f;
			
			rynx::math::rand64 m_random;
			rynx::type_index::virtual_type m_virtual_type_id;
			rynx::type_index::virtual_type m_virtual_type_id2;

			minilisk_test_spawner_logic(
				std::shared_ptr<rynx::mesh_collection> meshes,
				rynx::collision_detection::category_id dynamic
			) : m_meshes(meshes), dynamic(dynamic) {}

			virtual void onFrameProcess(rynx::scheduler::context& scheduler, float /* dt */) override {
				if ((++frameCount % how_often_to_spawn) == 0) {
					scheduler.add_task("create monsters", [this](
						rynx::ecs::edit_view<
							game::components::minilisk,
							game::health,
							rynx::components::collisions,
							rynx::components::position,
							rynx::components::motion,
							rynx::components::physical_body,
							rynx::components::radius,
							rynx::components::color,
							rynx::components::dampening,
							rynx::components::boundary,
							rynx::components::phys::joint,
							rynx::components::frame_collisions,
							rynx::components::light_omni,
							rynx::components::mesh,
							rynx::components::light_directed,
							rynx::matrix4> ecs) {
						if (m_virtual_type_id == rynx::type_index::virtual_type::invalid) {
							m_virtual_type_id = ecs.create_virtual_type();
							m_virtual_type_id2 = ecs.create_virtual_type();
						}

						for (int i = 0; i < 1; ++i) {
							float x = x_spawn + m_random(0.0f, 4.0f);
							float y = +100.0f + m_random(0.0f, 4.0f);
							if (true || m_random() > 0.5f) {
								auto id1 = ecs.create(
									rynx::components::position({x, y, 0 }),
									rynx::components::motion(),
									rynx::components::physical_body(5.0f, 15.0f, 0.3f, 1.0f),
									rynx::components::radius(2.0f),
									rynx::components::collisions{ dynamic.value },
									rynx::components::color(),
									rynx::components::dampening({ 0.97f, 0.997f }),
									rynx::components::mesh{ m_meshes->get("ball") },
									rynx::type_index::virtual_type{ m_virtual_type_id },
									rynx::components::frame_collisions(),
									rynx::matrix4()
								);

								if (m_random() > 0.75f) {
									rynx::components::light_omni light;
									light.color = rynx::floats4(m_random(0.0f, 1.0f), m_random(0.0f, 1.0f), m_random(0.0f, 1.0f), m_random(1.0f, 20.0f));
									light.ambient = m_random(0.0f, 0.1f);
									ecs.attachToEntity(id1, light);
								}
								else if (m_random() > 0.95f) {
									rynx::components::light_directed light;
									light.color = rynx::floats4(m_random(0.0f, 1.0f), m_random(0.0f, 1.0f), m_random(0.0f, 1.0f), m_random(1.0f, 20.0f));
									light.ambient = m_random(0.0f, 0.1f);
									light.direction = rynx::vec3f(1, 1, 0).normalize();
									light.edge_softness = m_random(0.5f, 5.0f);
									light.angle = m_random(1.5f, 4.14f);
									ecs.attachToEntity(id1, light);
								}
								/*
								for (int k = 1; k < 30; ++k) {
									auto id2 = ecs.create(
										rynx::components::position({ x + 3.5f * k, y + 3.5f * k, 0 }),
										rynx::components::motion(),
										rynx::components::physical_body(5.0f, 15.0f, 0.3f, 1.0f),
										rynx::components::radius(2.0f),
										rynx::components::collisions{ dynamic.value },
										rynx::components::color(),
										rynx::components::dampening({ 0.80f, 0.90f }),
										rynx::components::mesh{ m_meshes->get("square_empty") },
										rynx::type_index::virtual_type{ m_virtual_type_id2 },
										rynx::components::frame_collisions(),
										rynx::matrix4()
									);

									rynx::components::phys::joint joint;
									joint.connect_with_rod()
										.rotation_free();
									
									joint.id_a = id1;
									joint.id_b = id2;
									
									joint.point_a = vec3<float>(0, 0, 0);
									joint.point_b = vec3<float>(0, 0, 0);
									joint.length = 5.1f;
									joint.strength = 25.0f;
									ecs.create(joint);

									id1 = id2;
								}
								*/
							}
							else {
								float edge_length = 10.5f + 15.0f * m_random();
								float mass = 5000.0f;
								auto rectangle_moment_of_inertia = [](float mass, float width, float height) { return mass / 12.0f * (height * height + width * width); };
								float radius = rynx::math::sqrt_approx(2 * edge_length * edge_length);
								ecs.create(
									rynx::components::position(rynx::vec3<float>(x, y, 0.0f), i * 2.0f),
									rynx::components::collisions{ dynamic.value },
									rynx::components::boundary({ rynx::Shape::makeBox(edge_length).generateBoundary_Outside(1.0f) }),
									rynx::components::radius(radius),
									rynx::components::physical_body(mass, rectangle_moment_of_inertia(mass, edge_length, edge_length), 0.2f, 1.0f),
									rynx::components::color({ 1,1,0,1 }),
									rynx::components::motion({ 0, 0, 0 }, 0),
									rynx::components::dampening({ 0.2f, 1.f }),
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
