#pragma once

#include <rynx/tech/ecs.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/application/components.hpp>

#include <rynx/scheduler/context.hpp>

namespace game {
	struct ballCreatingRuleSet : public rynx::application::logic::iruleset {
		ballCreatingRuleSet(rynx::collision_detection::category_id category) : m_category(category) {}
		virtual ~ballCreatingRuleSet() {}

		rynx::collision_detection::category_id m_category;

		void createBall(rynx::ecs& ecs) {
			vec3<float> dir(
				(float(rand()) / RAND_MAX - 0.5f),
				(float(rand()) / RAND_MAX - 0.5f),
				0.0f
			);
			dir.normalizeApprox();
			float dist = float(rand()) / RAND_MAX * 3.0f + 10.0f;

			ecs.create(
				rynx::components::position(dir * dist),
				rynx::components::motion(),
				rynx::components::mass({ 1.0f }),
				rynx::components::radius(0.2f),
				rynx::components::collision_category(m_category),
				rynx::components::color()
			);
		}

		virtual void onFrameProcess(rynx::scheduler::context& scheduler) override {
			scheduler.add_task("createBalls", [this](rynx::ecs& ecs) {
				createBall(ecs);
				createBall(ecs);
				createBall(ecs);
			});
		}
	};
}