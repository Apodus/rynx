#pragma once

#include <rynx/application/logic.hpp>
#include <rynx/tech/ecs.hpp>
#include <vector>

namespace game {
	namespace components {
		struct weapon {
			float range = 10;
			float projectileSpeed = 20;

			float clipSizeMax = 75;
			float clipSizeCurrent = 75;

			float reloadTimeCurrent = 0;
			float reloadTimeMax = 1.5f;

			float cooldownMax = 0.04f;
			float cooldownCurrent = 0;

			int32_t bulletsPerShot = 1;
			float bulletSpread = 0.1f;
		};

		struct bullet {
			float damage = 12.0f;
		};
	}

	namespace actions {
		struct fire_weapon : public rynx::application::logic::iaction {
			fire_weapon(rynx::ecs::id id, vec3<float> target, rynx::collision_detection::category_id collisionCategory) : collisionCategory(collisionCategory), entity(id), targetPos(target) {}

			rynx::collision_detection::category_id collisionCategory;
			rynx::ecs::id entity;
			vec3<float> targetPos;

			virtual void apply(rynx::ecs& ecs) const {
				auto shooter = ecs[entity];
				game::components::weapon& w = shooter.get<game::components::weapon>();
				if (w.cooldownCurrent <= 0 && w.clipSizeCurrent > 0 && w.reloadTimeCurrent <= 0) {
					w.cooldownCurrent = w.cooldownMax;
					if (--w.clipSizeCurrent == 0) {
						w.reloadTimeCurrent = w.reloadTimeMax;
					}

					auto shooterPos = shooter.get<rynx::components::position>();
					auto shooterMotion = shooter.get<rynx::components::motion>();
					vec3<float> direction = targetPos - shooterPos.value;
					direction.normalizeApprox();
					auto normalizedDirection = direction;
					direction *= w.projectileSpeed * 0.1f;
					
					// todo: need usable pseudo random source.
					float spreadForOne = 2.0f * ((float(rand()) / RAND_MAX) - 0.5f) * w.bulletSpread;
					math::rotateXY(direction, spreadForOne);

					float bulletRadius = 0.3f;

					ecs.create(
						rynx::components::lifetime(3.0f),
						rynx::components::position(shooterPos.value + direction + shooterMotion.velocity * 1.1f + normalizedDirection * (shooter.get<rynx::components::radius>().r + bulletRadius) * 1.1f),
						rynx::components::radius(bulletRadius),
						rynx::components::projectile(),
						rynx::components::collision_category(collisionCategory),
						rynx::components::color({ 1, 0, 0, 1 }),
						rynx::components::motion({ {direction + shooterMotion.velocity }, {} }),
						rynx::components::frame_collisions(),
						game::components::bullet({ 11 })
					);
				}
			}
		};
	}

	namespace logic {
		struct shooting_logic : public rynx::application::logic::iruleset {
			int32_t shootKey;
			rynx::collision_detection::category_id projectiles;
			shooting_logic(rynx::input::mapped_input& input, rynx::collision_detection::category_id projectiles) : projectiles(projectiles) {
				shootKey = input.generateAndBindGameKey(input.getMouseKeyPhysical(0), "shoot");
			}

			virtual std::vector<std::unique_ptr<rynx::application::logic::iaction>> onInput(rynx::input::mapped_input& input, const rynx::ecs& ecs) override {
				int localPlayerId = 1; // TODO: Fix hard coded local player id
				if (ecs.exists(localPlayerId)) {
					auto localPlayerEntity = ecs[localPlayerId];

					if (localPlayerEntity.has<game::components::weapon>()) {
						const game::components::weapon& w = localPlayerEntity.get<game::components::weapon>();

						// only emit the action if it is possible to execute.
						if (w.cooldownCurrent <= 0 && w.clipSizeCurrent > 0 && w.reloadTimeCurrent <= 0) {
							if (input.isKeyDown(shootKey)) {
								std::vector<std::unique_ptr<rynx::application::logic::iaction>> result;
								result.emplace_back(std::make_unique<actions::fire_weapon>(localPlayerId, input.mouseWorldPosition(), projectiles));
								return result;
							}
						}
					}
				}
				// otherwise no action.
				return {};
			}

			virtual void onFrameProcess(rynx::scheduler::context& scheduler, float dt) override {
				scheduler.add_task("tick weapons", [dt](rynx::ecs::view<game::components::weapon> ecs) {
					ecs.for_each([dt](game::components::weapon& weapon) {
						weapon.cooldownCurrent -= dt; // TODO: should we just express these as number of ticks, instead of seconds?
						weapon.reloadTimeCurrent -= dt;
						if (weapon.reloadTimeCurrent <= 0 && weapon.clipSizeCurrent == 0) {
							weapon.clipSizeCurrent = weapon.clipSizeMax;
						}
					});
				});
			}
		};
	}
}