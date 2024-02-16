#include <rynx/rulesets/lifetime.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/tech/components.hpp>

// generators
#include <rynx/ecs/scene_serialization.hpp>

void rynx::ruleset::lifetime_updates::onFrameProcess(rynx::scheduler::context& context, float dt) {
	context.add_task("lifetimes", [dt](rynx::scheduler::task& context, rynx::ecs::view<rynx::components::entity::lifetime> ecs) {
		std::vector<rynx::ecs::id> ids = ecs.query().ids_if([dt](rynx::components::entity::lifetime& time) {
			time.value -= dt;
			return time.value <= 0.0f;
		});

		if (!ids.empty()) {
			context.extend_task_execute_sequential("lifetimes over", [ids = std::move(ids)](rynx::ecs& whole_ecs) {
				for (auto&& id : ids)
					whole_ecs.attachToEntity(id, rynx::components::entity::dead());
			});
		}
	});



	context.add_task("generators", [dt](rynx::scheduler::task& context, rynx::ecs::view<rynx::components::transform::position, rynx::components::logic::interval_generator, const rynx::components::logic::generate_on_start> ecs) {
		std::vector<std::tuple<rynx::components::transform::position, rynx::scene_id>> scenes_to_gen;
		ecs.query().for_each([&scenes_to_gen, dt](
			rynx::components::transform::position pos,
			rynx::components::logic::interval_generator& generator)
		{
			if (generator.target) {
				generator.time_until_next -= dt;
				while (generator.time_until_next < 0.0f) {
					generator.time_until_next += 0.3f;
					scenes_to_gen.emplace_back(pos, generator.target);
				}
			}
		});

		ecs.query().for_each([&scenes_to_gen, dt](
			rynx::id entity_id,
			rynx::components::transform::position pos,
			rynx::components::logic::generate_on_start generator)
			{
				if (generator.target) {
					scenes_to_gen.emplace_back(pos, generator.target);
				}
			});

		std::vector<rynx::id> on_start_generated = ecs.query().in<rynx::components::logic::generate_on_start>().ids();
		if (!on_start_generated.empty()) {
			context.extend_task_independent([entities_to_remove = std::move(on_start_generated)](rynx::ecs::edit_view<rynx::components::logic::generate_on_start> ecs) {
				for (auto id : entities_to_remove) {
					ecs[id].remove<rynx::components::logic::generate_on_start>();
				}
			});
		}

		if (!scenes_to_gen.empty()) {
			context.extend_task_independent([scenes = std::move(scenes_to_gen)](
				rynx::scheduler::task& context,
				rynx::scheduler::context& ctx,
				rynx::reflection::reflections& reflection,
				rynx::filesystem::vfs& vfs,
				rynx::scenes& scene_manager,
				rynx::ecs& ecs) mutable
			{
				for (auto[pos, scene_id] : scenes) {
					ecs.create(pos, rynx::components::scene::link(scene_id));
				}

				auto all_ids = rynx::ecs_detail::scene_serializer(&ecs).load_subscenes(reflection, vfs, scene_manager);
				ecs.post_deserialize_init(ctx, all_ids);
			});
		}
	});
}