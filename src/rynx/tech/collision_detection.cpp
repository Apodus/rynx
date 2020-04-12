#include <rynx/tech/collision_detection.hpp>
#include <rynx/tech/sphere_tree.hpp>
#include <rynx/tech/components.hpp>

rynx::collision_detection::category_id rynx::collision_detection::add_category() {
	m_sphere_trees.emplace_back(std::make_unique<sphere_tree>());
	return category_id(int32_t(m_sphere_trees.size()) - 1);
}

rynx::collision_detection& rynx::collision_detection::enable_collisions_between(category_id category1, category_id category2) {
	rynx_assert(!(category1.m_ignore_collisions && category2.m_ignore_collisions), "checking collisions between two categories who both ignore collisions, makes no sense");
	if (category1.m_ignore_collisions) {
		m_collision_checks.emplace_back(
			m_sphere_trees[category2.value].get(),
			m_sphere_trees[category1.value].get(),
			check_type::b_is_static
		);
	}
	else {
		m_collision_checks.emplace_back(
			m_sphere_trees[category1.value].get(),
			m_sphere_trees[category2.value].get(),
			category2.m_ignore_collisions ? check_type::b_is_static : check_type::both_are_dynamic
		);
	}
	return *this;
}

void rynx::collision_detection::update() {
	for (auto& tree : m_sphere_trees) {
		tree->update();
	}
}

void rynx::collision_detection::update_parallel(rynx::scheduler::task& task_context) {
	for (auto& tree : m_sphere_trees) {
		tree->update_parallel(task_context);
	}
}

void rynx::collision_detection::track_entities(rynx::scheduler::task& task_context) {
	struct tracked_by_collisions {};
	task_context.extend_task_independent([](rynx::scheduler::task& task_context, rynx::ecs::edit_view<rynx::components::collisions, tracked_by_collisions, rynx::components::position, rynx::components::radius> ecs) {
		auto collisions = ecs.query()
			.in<rynx::components::physical_body>()
			.notIn<tracked_by_collisions>()
			.gather<rynx::ecs::id, rynx::components::collisions, rynx::components::position, rynx::components::radius>();
		
		for (auto&& col : collisions) {
			ecs.attachToEntity(std::get<0>(col), tracked_by_collisions{});
		}
		
		task_context.extend_task_independent([collisions = std::move(collisions)](rynx::collision_detection& detection) {
			for (auto&& col : collisions) {
				detection.m_sphere_trees[std::get<1>(col).category]->insert_entity(std::get<0>(col).value, std::get<2>(col).value, std::get<3>(col).r);
			}
		});
	});
}

void rynx::collision_detection::update_entities(rynx::scheduler::task& task_context, float dt) {
	task_context.extend_task_independent([dt](
		rynx::scheduler::task& task_context,
		rynx::collision_detection& detection,
		rynx::ecs::view<
			const rynx::components::collisions,
			const rynx::components::position,
			const rynx::components::motion,
			const rynx::components::radius,
			const rynx::components::physical_body,
			const rynx::components::projectile> ecs)
	{
		auto non_projectiles = ecs.query()
			.notIn<rynx::components::projectile>()
			.in<rynx::components::physical_body>()
			.for_each_parallel(task_context, [&detection](
				rynx::ecs::id id,
				rynx::components::collisions col,
				rynx::components::position pos,
				rynx::components::radius r)
		{
			detection.m_sphere_trees[col.category]->update_entity(id.value, pos.value, r.r);
		});

		auto projectiles = ecs.query()
			.in<rynx::components::physical_body, rynx::components::projectile>()
			.for_each_parallel(task_context, [&detection, dt](
				rynx::ecs::id id,
				rynx::components::collisions col,
				rynx::components::position pos,
				rynx::components::motion motion,
				rynx::components::radius r)
		{
			detection.m_sphere_trees[col.category]->update_entity(id.value, pos.value - motion.velocity * dt, r.r);
		});
	});
}

void rynx::collision_detection::erase(uint64_t entityId, category_id from) {
	m_sphere_trees[from.value]->eraseEntity(entityId);
}

rynx::sphere_tree* rynx::collision_detection::get(category_id category) { return m_sphere_trees[category.value].get(); }