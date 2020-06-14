#include <rynx/rulesets/lifetime.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/tech/components.hpp>

void rynx::ruleset::lifetime_updates::onFrameProcess(rynx::scheduler::context& context, float dt) {
	
	context.add_task("lifetimes", [dt](rynx::scheduler::task& context, rynx::ecs::view<rynx::components::lifetime> ecs) {
		std::vector<rynx::ecs::id> ids = ecs.query().ids_if([dt](rynx::components::lifetime& time) {
			time.value -= dt;
			return time.value <= 0.0f;
		});

		context.extend_task_execute_sequential("lifetimes over", [ids = std::move(ids)](rynx::ecs& whole_ecs) {
			for (auto&& id : ids)
				whole_ecs.attachToEntity(id, rynx::components::dead());
		});
	});
}