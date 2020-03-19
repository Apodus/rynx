#pragma once

#include <rynx/tech/ecs.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/application/components.hpp>

#include <rynx/scheduler/context.hpp>

namespace game {
	struct ballCreatingRuleSet : public rynx::application::logic::iruleset {
		ballCreatingRuleSet(rynx::collision_detection::category_id cat_id) : m_category(cat_id) {}
		virtual ~ballCreatingRuleSet() {}

		rynx::collision_detection::category_id m_category;
		float spawnInterval = 1.0f;
		float currentTime = 1.0f;

		void createBall(rynx::ecs& ecs) {
			vec3<float> dir(
				(float(rand()) / RAND_MAX - 0.5f),
				(float(rand()) / RAND_MAX - 0.5f),
				0.0f
			);
			dir.normalize();
			float dist = float(rand()) / RAND_MAX * 3.0f + 10.0f;

			ecs.create(
				rynx::components::position(dir * dist),
				rynx::components::motion(),
				rynx::components::physical_body({ 1.0f / 5.0f, 1.0f / 15.0f, 0.3f, 1.0f }),
				rynx::components::radius(0.2f),
				rynx::components::color(),
				rynx::components::collisions{ m_category.value }
			);
		}

		virtual void onFrameProcess(rynx::scheduler::context& scheduler, float dt) override {
			currentTime -= dt;
			if (currentTime < 0) {
				currentTime += spawnInterval;

				scheduler.add_task("createBalls", [this](rynx::ecs& ecs) {
					createBall(ecs);
					createBall(ecs);
					createBall(ecs);
				});
			}

		}
	};
}