
#include <rynx/rulesets/frustum_culling.hpp>

#include <rynx/application/components.hpp>
#include <rynx/tech/math/vector.hpp>

#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <rynx/tech/math/matrix.hpp>
#include <rynx/graphics/camera/frustumr.hpp>
#include <rynx/graphics/camera/camera.hpp>


void rynx::ruleset::frustum_culling::on_entities_erased(rynx::scheduler::context& context, const std::vector<rynx::ecs::id>& ids) {
	auto& ecs = context.get_resource<rynx::ecs>();
	for (auto id : ids) {
		m_in_frustum.eraseEntity(id.value);
		m_out_frustum.eraseEntity(id.value);
	}
}

void rynx::ruleset::frustum_culling::onFrameProcess(rynx::scheduler::context& context, float /* dt */) {
	
	auto update_new_entities = context.add_task("add new entities to frustum culling structures", [this](
		rynx::ecs::edit_view<
		entity_tracked_by_frustum_culling,
		const components::frustum_culled,
		const components::radius,
		const components::position> ecs,
		rynx::scheduler::task& task_context)
		{
			auto ids = ecs.query().notIn<entity_tracked_by_frustum_culling>().in<components::position, components::radius>().ids();
			auto entity_data_vector = ecs.query()
				.notIn<entity_tracked_by_frustum_culling>()
				.gather<rynx::ecs::id, components::position, components::radius>();
			
			for (auto&& data : entity_data_vector) {
				m_in_frustum.insert_entity(std::get<0>(data).value, std::get<1>(data).value, std::get<2>(data).r);
			}
			
			if (!ids.empty()) {
				for (auto id : ids) {
					ecs.attachToEntity(id, entity_tracked_by_frustum_culling());
				}
			}
		}
	);

	auto update_existing_entities = context.add_task("frustum culling",
		[this](rynx::ecs::view<
			const components::radius,
			const components::position> ecs,
			rynx::scheduler::task& task_context)
		{
			auto update_positions_to_sphere_tree = task_context.extend_task_independent([this](
				rynx::scheduler::task& task_context,
				rynx::ecs::view<
				const entity_tracked_by_frustum_culling,
				const components::frustum_culled,
				const rynx::components::position,
				const rynx::components::radius> ecs)
				{
					task_context & ecs.query()
						.in<entity_tracked_by_frustum_culling>()
						.notIn<components::frustum_culled>()
						.for_each_parallel(task_context, [this](rynx::ecs::id id, rynx::components::position pos, rynx::components::radius r) {
						m_in_frustum.update_entity(id.value, pos.value, r.r);
					});

					task_context & ecs.query()
						.in<entity_tracked_by_frustum_culling, components::frustum_culled>()
						.for_each_parallel(task_context, [this](rynx::ecs::id id, rynx::components::position pos, rynx::components::radius r) {
						m_out_frustum.update_entity(id.value, pos.value, r.r);
					});
				});
		
			auto culling_task = task_context.make_task("frustum culling",
				[this](rynx::ecs::view<
					const components::radius,
					const components::position> ecs,
					rynx::scheduler::task& task_context)
				{
					rynx_profile("culling", "update sphere trees");
					auto bar1 = m_in_frustum.update_parallel(task_context);
					auto bar2 = m_out_frustum.update_parallel(task_context);

					auto frustum_migrates = task_context.make_task("find frustum migrates", [this](rynx::scheduler::task& task_context) {
						const rynx::matrix4& view_matrix = m_pCamera->getView();
						const rynx::matrix4& projection_matrix = m_pCamera->getProjection();
						rynx::matrix4 view_projection_matrix = projection_matrix * view_matrix;

						frustum f;
						f.set_viewprojection(view_projection_matrix);

						std::vector<uint64_t> move_to_outside;
						std::vector<uint64_t> move_to_inside;

						m_in_frustum.in_volume(
							[&f](vec3f pos, float r) {
								// for the tree nodes we check if the sphere node has any parts outside frustum.
								// if yes, then it is possible that there exists an entity that is completely outside of frustum.
								return !f.is_sphere_inside(pos, r);
							},
							[&move_to_outside, &f](uint64_t id, vec3f pos, float r) {
								// for actual entities we do more rigorous test. has to be completely outside frustum.
								if (f.is_sphere_outside(pos, r)) {
									move_to_outside.emplace_back(id);
								}
							}
						);

						m_out_frustum.in_volume(
							[&f](vec3f pos, float r) {
								return !f.is_sphere_outside(pos, r);
							},
							[&move_to_inside](uint64_t id, vec3f, float) {
								move_to_inside.emplace_back(id);
							}
						);

						if (!move_to_inside.empty() || !move_to_outside.empty()) {
							task_context.extend_task_independent("apply frustum migrates", [this, move_to_inside = std::move(move_to_inside), move_to_outside = std::move(move_to_outside)](
								rynx::ecs::edit_view<components::frustum_culled> ecs) {
								for (auto id : move_to_inside) {
									ecs[id].remove<components::frustum_culled>();
									auto id_data = m_out_frustum.eraseEntity(id);
									m_in_frustum.insert_entity(id, id_data.first, id_data.second);
								}
								for (auto id : move_to_outside) {
									ecs[id].add(components::frustum_culled());
									auto id_data = m_in_frustum.eraseEntity(id);
									m_out_frustum.insert_entity(id, id_data.first, id_data.second);
								}
							});
						}
					});
					
					frustum_migrates->depends_on(bar1);
					frustum_migrates->depends_on(bar2);
				}
			);

			culling_task->depends_on(update_positions_to_sphere_tree);
		}
	);

	update_existing_entities->depends_on(update_new_entities);
}