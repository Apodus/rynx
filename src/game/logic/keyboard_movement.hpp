#pragma once

#include <rynx/application/logic.hpp>
#include <rynx/tech/ecs.hpp>

#include <rynx/tech/math/vector.hpp>
#include <rynx/application/components.hpp>
#include <rynx/input/mapped_input.hpp>

#include <game/gametypes.hpp>

#include <vector>

namespace game {
	namespace components {
		// could have some boost components or whatever.
	}

	namespace actions {
		struct movement : public rynx::application::logic::iaction {
			movement(rynx::ecs::id id, vec3<float> acceleration) : entity(id), acceleration(acceleration) {}

			rynx::ecs::id entity;
			vec3<float> acceleration;

			virtual void apply(rynx::ecs& ecs) const {
				if (ecs.exists(entity)) {
					auto unit = ecs[entity];
					unit.get<rynx::components::motion>().velocity += acceleration;
				}
			}
		};
	}

	namespace logic {
		struct keyboard_movement_logic : public rynx::application::logic::iruleset {

			int32_t moveUp;
			int32_t moveDown;
			int32_t moveLeft;
			int32_t moveRight;

			keyboard_movement_logic(rynx::input::mapped_input& input) {
				moveLeft = input.generateAndBindGameKey('A', "move_left");
				moveRight = input.generateAndBindGameKey('D', "move_right");
				moveUp = input.generateAndBindGameKey('W', "move_up");
				moveDown = input.generateAndBindGameKey('S', "move_down");
			}

			virtual std::vector<std::unique_ptr<rynx::application::logic::iaction>> onInput(rynx::input::mapped_input& input, const rynx::ecs& ecs) override {
				int localPlayer = 1;
				std::vector<std::unique_ptr<rynx::application::logic::iaction>> result;
				
				if (ecs.exists(localPlayer)) {
					auto localPlayerEntity = ecs[localPlayer];

					vec3<float> total;
					if (input.isKeyDown(moveLeft)) {
						total.x -= 0.025f;
					}
					if (input.isKeyDown(moveRight)) {
						total.x += 0.025f;
					}
					if (input.isKeyDown(moveUp)) {
						total.y += 0.025f;
					}
					if (input.isKeyDown(moveDown)) {
						total.y -= 0.025f;
					}

					if (total.lengthSquared() > 0) {
						result.emplace_back(std::make_unique<actions::movement>(localPlayer, total));
					}
				}
				return result;
			}

			virtual void onFrameProcess(rynx::scheduler::context&, float) override {}
		};
	}
}
