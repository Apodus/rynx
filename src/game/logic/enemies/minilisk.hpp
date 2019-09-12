#pragma once

#include <rynx/application/components.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/tech/ecs.hpp>
#include <vector>

#include <rynx/tech/math/vector.hpp>
#include <rynx/graphics/mesh/math.hpp>

#include <game/gametypes.hpp>

namespace game {
	namespace components {
		struct minilisk {
			float jumpCooldown = 0;
			float jumpCooldownMax = 0.40f;
			float jumpAngleSpread = 1.1f;
		};
	}

	namespace logic {
		struct minilisk_logic : public rynx::application::logic::iruleset {
			minilisk_logic() {}
			
			virtual void onFrameProcess(rynx::scheduler::context& scheduler) override {
				scheduler.add_task("minilisk logic", [](rynx::ecs::edit_view<
					const game::hero,
					const rynx::components::radius,
					const rynx::components::position,
					game::components::minilisk,
					rynx::components::motion,
					game::dead,
					game::health> ecs
					) {
					std::vector<rynx::ecs::id> heroes;
					ecs.for_each([&heroes](rynx::ecs::id id, const game::hero&) {
						heroes.emplace_back(id);
					});

					if (heroes.empty()) {
						return; // no players, nothing to do.
					}

					ecs.for_each([&heroes, &ecs](game::components::minilisk& enemy, const rynx::components::position& pos, rynx::components::motion& m, const rynx::components::radius& r) {
						enemy.jumpCooldown -= 0.016f;
						if (enemy.jumpCooldown <= 0) {
							enemy.jumpCooldown = enemy.jumpCooldownMax;

							// could select closest hero, or random hero, or whatever.
							auto heroEntity = ecs[heroes[0]];
							const auto& heroPos = heroEntity.get<const rynx::components::position>();
							auto direction = heroPos.value - pos.value;

							float maxDistance = heroEntity.get<const rynx::components::radius>().r + r.r + 2.7f;
							if (direction.lengthSquared() < maxDistance * maxDistance) {
								auto& hp = heroEntity.get<game::health>();
								hp.currentHp -= 5;
								if (hp.currentHp <= 0) {
									ecs.attachToEntity(heroes[0].value, game::dead());
								}
							}

							direction.normalizeApprox();
							float spreadMultiplier = 2.0f * (float(rand()) / RAND_MAX) - 1;
							math::rotateXY(direction, spreadMultiplier * enemy.jumpAngleSpread);

							m.acceleration += direction * 0.35f;
						}
					});
				});
			}
		};

		struct bullet_hitting_logic : public rynx::application::logic::iruleset {
			rynx::collision_detection& collisionDetection;
			rynx::collision_detection::category_id dynamics;
			rynx::collision_detection::category_id statics;
			rynx::collision_detection::category_id projectiles;

			std::vector<uint32_t> bulletsCollided;

			bullet_hitting_logic(
				rynx::collision_detection& collisionDetection,
				rynx::collision_detection::category_id dynamics,
				rynx::collision_detection::category_id statics,
				rynx::collision_detection::category_id projectiles
			) : collisionDetection(collisionDetection),
			dynamics(dynamics),
			statics(statics),
			projectiles(projectiles)
			{}
			
			virtual void onFrameProcess(rynx::scheduler::context& context) override {
				context.add_task("hehe", [this](
					rynx::ecs::view<
						const rynx::components::position,
						rynx::components::radius> ecs
					)
					{
						ecs.query().in<game::components::minilisk>().execute(
							[](const rynx::components::position& p, rynx::components::radius& r) {
								r.r = 1.0f + 0.5f * std::sin((p.value.x + p.value.y) * 0.55f);
							}
						);
					}
				);
				
				context.add_task("handle bullet collisions", [this](
					rynx::ecs::view<
						const rynx::components::projectile,
						const game::components::bullet,
						const rynx::components::position,
						const rynx::components::motion,
						const rynx::components::frame_collisions> ecs,
					rynx::scheduler::task& currentTask)
				{
					struct bullet_hit {
						rynx::ecs::id targetId;
						rynx::ecs::id bulletId;
						float damage;
					};

					std::vector<bullet_hit> hits;

					ecs.for_each([&](rynx::ecs::id id, const rynx::components::frame_collisions& collisions, const rynx::components::projectile&, const game::components::bullet& bullet, const rynx::components::position& pos, const rynx::components::motion& m) {
						if (collisions.collisions.empty())
							return;

						auto prevPos = pos.value - m.velocity;
						size_t closestIndex = 0;
						float closestDistance = 73496798.0f;

						for (size_t i = 0; i < collisions.collisions.size(); ++i) {
							float d = (collisions.collisions[i].collisionPoint - prevPos).lengthSquared();
							if (d < closestDistance) {
								closestDistance = d;
								closestIndex = i;
							}
						}

						bullet_hit bhit;

						// allow only one hit per bullet?
						auto& col = collisions.collisions[closestIndex];
						bhit.targetId = col.idOfOther;
						bhit.bulletId = id;
						bhit.damage = bullet.damage;
						hits.emplace_back(bhit);
					});

					if (!hits.empty()) {
						currentTask.extend_task([hits = std::move(hits)](rynx::ecs::edit_view<game::health, game::dead> ecs) {
							for (auto&& hit : hits) {
								if (ecs[hit.targetId].has<game::health>()) {
									game::health& hp = ecs[hit.targetId].get<game::health>();
									hp.currentHp -= hit.damage;
									if (hp.currentHp <= 0) {
										ecs.attachToEntity(hit.targetId, game::dead());
									}
								}

								ecs.attachToEntity(hit.bulletId, game::dead());
							}
						});
					}
				});
			}
		};
	}
}
