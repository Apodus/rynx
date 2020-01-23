
#pragma once

#include <rynx/application/components.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/tech/math/vector.hpp>

#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <rynx/tech/math/matrix.hpp>
#include <rynx/graphics/camera/frustumr.hpp>
#include <rynx/graphics/camera/camera.hpp>

#include <rynx/tech/sphere_tree.hpp>

// TODO: MEGATODO this one.
// TODO: System needs to be informed of dead entities. ruleset api should reflect that.

namespace rynx {
	namespace ruleset {
		class frustum_culling : public application::logic::iruleset {
			struct entity_tracked_by_frustum_culling {};

		public:
			frustum_culling(std::shared_ptr<Camera> camera) 
				: m_pCamera(std::move(camera))
			{}
			virtual ~frustum_culling() {}
			virtual void onFrameProcess(rynx::scheduler::context& context, float /* dt */) override {
				
				auto update_new_entities = context.add_task("add new entities to frustum culling structures", [this](
					rynx::ecs::view<
					const entity_tracked_by_frustum_culling,
					const components::frustum_culled,
					const components::radius,
					const components::position> ecs,
					rynx::scheduler::task& task_context) mutable
					{
						auto ids = ecs.query().notIn<entity_tracked_by_frustum_culling>().in<components::position, components::radius>().ids();
						ecs.query()
							.notIn<entity_tracked_by_frustum_culling>()
							.for_each([this](rynx::ecs::id id, components::position pos, components::radius r)
						{
							m_in_frustum.updateEntity(pos.value, r.r, id.value);
						});
						
						if (!ids.empty()) {
							task_context.extend_task_execute_sequential(
								[ids = std::move(ids), this](rynx::ecs::edit_view<entity_tracked_by_frustum_culling> ecs) {
									for (auto id : ids) {
										ecs.attachToEntity(id, entity_tracked_by_frustum_culling());
									}
								}
							);
						}
					}
				);
				
				auto update_existing_entities = context.add_task("frustum culling",
					[this](rynx::ecs::view<
						const components::radius,
						const components::position> ecs,
						rynx::scheduler::task& task_context)
					{
						{
							rynx_profile("culling", "update culled sphere tree items");
							ecs.query()
								.in<components::frustum_culled>()
								.for_each([this](rynx::ecs::id id, components::position pos, components::radius r) {
								m_out_frustum.updateEntity(pos.value, r.r, id.value);
							});
						}

						{
							rynx_profile("culling", "update visible sphere tree items");
							ecs.query()
								.notIn<components::frustum_culled>()
								.for_each([this](rynx::ecs::id id, components::position pos, components::radius r) {
								m_in_frustum.updateEntity(pos.value, r.r, id.value);
							});
						}
						
						rynx_profile("culling", "update sphere trees");
						auto bar1 = m_in_frustum.update_parallel(task_context);
						auto bar2 = m_out_frustum.update_parallel(task_context);

						// m_in_frustum.update();
						// m_out_frustum.update();

						task_context.make_task("find frustum migrates", [this](rynx::scheduler::task& task_context) {
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
									return f.is_sphere_not_outside(pos, r);
								},
								[&move_to_inside](uint64_t id, vec3f, float) {
									move_to_inside.emplace_back(id);
								}
							);

							if (!move_to_inside.empty() || !move_to_outside.empty()) {
								task_context.extend_task([this, move_to_inside = std::move(move_to_inside), move_to_outside = std::move(move_to_outside)](rynx::ecs::edit_view<components::frustum_culled> ecs) {
									for (auto id : move_to_inside) {
										if (ecs.exists(id)) {
											ecs[id].remove<components::frustum_culled>();
										}
										m_out_frustum.eraseEntity(id);
									}
									for (auto id : move_to_outside) {
										if (ecs.exists(id)) {
											ecs[id].add(components::frustum_culled());
										}
										m_in_frustum.eraseEntity(id);
									}
								});
							}
						//	});
						})->depends_on(bar1).depends_on(bar2);
					}
				);

				update_existing_entities->depends_on(update_new_entities);
			}

		private:
			rynx::sphere_tree m_in_frustum;
			rynx::sphere_tree m_out_frustum;
			std::shared_ptr<Camera> m_pCamera;
		};
	}
}
