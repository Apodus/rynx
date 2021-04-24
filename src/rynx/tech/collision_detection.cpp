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

void rynx::collision_detection::clear() {
	for (auto& tree : m_sphere_trees) {
		tree->clear();
	}
}

void rynx::collision_detection::update_sphere_trees() {
	for (auto& tree : m_sphere_trees) {
		tree->update();
	}
}

void rynx::collision_detection::update_sphere_trees_parallel(rynx::scheduler::task& task_context) {
	for (auto& tree : m_sphere_trees) {
		tree->update_parallel(task_context);
	}
}

void rynx::collision_detection::track_entities(rynx::scheduler::task& task_context) {
	task_context.extend_task_independent([](
		rynx::collision_detection& detection,
		rynx::ecs::edit_view<
			const tracked_by_collisions,
			const rynx::components::boundary,
			const rynx::components::collisions,
			const rynx::components::position,
			const rynx::components::radius> ecs) {
		{
			// cleanup removed collisions components
			auto collisions_erased_ids = ecs.query()
				.in<tracked_by_collisions>()
				.notIn<rynx::components::collisions>()
				.ids();
			
			// this should probably not happen outside editor context?
			for (auto id : collisions_erased_ids) {
				detection.editor_api().remove_collision_from_entity(ecs, id);
			}

			auto spheres = ecs.query()
				.in<rynx::components::physical_body, rynx::components::motion>()
				.notIn<rynx::components::projectile, rynx::components::boundary, tracked_by_collisions>()
				.for_each([&detection](
					rynx::ecs::id id,
					rynx::components::collisions col,
					rynx::components::position pos,
					rynx::components::radius r)
			{
				rynx_assert((id.value & rynx::collision_detection::mask_id) == id.value, "id out of bounds");
				uint64_t part_id = id.value | mask_kind_sphere;
				detection.m_sphere_trees[col.category]->insert_entity(part_id, pos.value, r.r);
			});

			auto boundaries = ecs.query()
				.in<rynx::components::physical_body, rynx::components::radius, rynx::components::position, rynx::components::boundary>()
				.notIn<rynx::components::projectile, tracked_by_collisions>()
				.for_each([&detection](
					rynx::ecs::id id,
					rynx::components::position pos,
					rynx::components::radius r,
					rynx::components::collisions col)
			{
				const uint64_t part_id = id.value | mask_kind_boundary;
				detection.m_sphere_trees[col.category]->insert_entity(part_id, pos.value, r.r);
			});

			auto projectiles = ecs.query()
				.in<rynx::components::physical_body, rynx::components::projectile>()
				.notIn<tracked_by_collisions>()
				.for_each([&detection](
					rynx::ecs::id id,
					rynx::components::collisions col,
					rynx::components::position pos,
					rynx::components::radius r)
			{
				rynx_assert((id.value & rynx::collision_detection::mask_id) == id.value, "id out of bounds");
				uint64_t part_id = id.value | mask_kind_projectile;
				detection.m_sphere_trees[col.category]->insert_entity(part_id, pos.value, r.r);
			});


			auto added_entities = ecs.query()
				.in<rynx::components::physical_body, rynx::components::collisions, rynx::components::position, rynx::components::radius>()
				.notIn<tracked_by_collisions>()
				.ids();

			for (auto id : added_entities) {
				ecs.attachToEntity(id.value, tracked_by_collisions{});
			}
		}
	});
}

void rynx::collision_detection::editor_api_t::update_entity_forced(rynx::ecs& ecs, rynx::ecs::id id) {
	auto entity = ecs[id];
	auto collisions = entity.get<const rynx::components::collisions>();
	auto radius = entity.get<const rynx::components::radius>();
	auto pos = entity.get<const rynx::components::position>();

	bool projectile = entity.has<rynx::components::projectile>();
	bool boundary = entity.has<rynx::components::boundary>();
	bool sphere = !(projectile | boundary);

	uint64_t part_id = projectile * mask_kind_projectile + boundary * mask_kind_boundary + sphere * mask_kind_sphere + id.value;

	// could be that we are running an editor or something, and our force updated entity has just been created
	// before running an iteration of collision detection updates, where this entity will become tracked.
	// if this is the case, then just don't do anything - let the normal path pick the entity up.
	// otherwise update immediately.
	if(m_host->m_sphere_trees[collisions.category]->contains(part_id))
		m_host->m_sphere_trees[collisions.category]->update_entity(part_id, pos.value, radius.r);
}

void rynx::collision_detection::editor_api_t::remove_collision_from_entity(
	rynx::ecs::edit_view<const tracked_by_collisions, const rynx::components::collisions> ecs,
	rynx::ecs::id id)
{
	for (auto& tree : m_host->m_sphere_trees) {
		auto try_erase = [&tree](auto id) {
			if (tree->contains(id)) {
				tree->eraseEntity(id);
			}
		};

		try_erase(id.value | mask_kind_sphere);
		try_erase(id.value | mask_kind_boundary);
		try_erase(id.value | mask_kind_projectile);
	}
	if (ecs[id].has<tracked_by_collisions, rynx::components::collisions>())
		ecs[id].remove<tracked_by_collisions, rynx::components::collisions>();
}

void rynx::collision_detection::editor_api_t::set_collision_category_for_entity(rynx::ecs& ecs, rynx::ecs::id id, category_id category) {
	remove_collision_from_entity(ecs, id);
	rynx::components::collisions col(category.value);
	ecs[id].add(col);

	bool prerequirements = ecs[id].has<
		rynx::components::physical_body,
		rynx::components::motion,
		rynx::components::radius,
		rynx::components::position>();

	if (!prerequirements)
		return;

	bool projectile = ecs[id].has<rynx::components::projectile>();
	bool boundary = ecs[id].has<rynx::components::boundary>();

	auto pos = ecs[id].get<const rynx::components::position>();
	auto r = ecs[id].get<const rynx::components::radius>();

	rynx_assert((id.value & rynx::collision_detection::mask_id) == id.value, "id out of bounds");
	
	if (prerequirements & projectile & !boundary) {
		const uint64_t part_id = id.value | mask_kind_projectile;
		m_host->m_sphere_trees[col.category]->insert_entity(part_id, pos.value, r.r);
	}
	else if (prerequirements & boundary) {
		const uint64_t part_id = id.value | mask_kind_boundary;
		m_host->m_sphere_trees[col.category]->insert_entity(part_id, pos.value, r.r);
	}
	else if (prerequirements) {
		const uint64_t part_id = id.value | mask_kind_sphere;
		m_host->m_sphere_trees[col.category]->insert_entity(part_id, pos.value, r.r);
	}
	
	ecs[id].add(tracked_by_collisions());
}


void rynx::collision_detection::update_entities(rynx::scheduler::task& task_context, float dt) {
	auto update_task = task_context.extend_task_independent([dt](
		rynx::scheduler::task& task_context,
		rynx::collision_detection& detection,
		rynx::ecs::view<
			const rynx::components::collisions,
			const rynx::components::position,
			const rynx::components::motion,
			const rynx::components::radius,
			const rynx::components::physical_body,
			const rynx::components::projectile,
			const rynx::components::boundary> ecs)
	{
		auto spheres = ecs.query()
			.in<rynx::components::physical_body, rynx::components::motion, tracked_by_collisions>()
			.notIn<rynx::components::projectile, rynx::components::boundary>()
			.for_each_parallel(task_context, [&detection](
				rynx::ecs::id id,
				rynx::components::collisions col,
				rynx::components::position pos,
				rynx::components::radius r)
		{
			rynx_assert((id.value & rynx::collision_detection::mask_id) == id.value, "id out of bounds");
			uint64_t part_id = id.value | mask_kind_sphere;
			detection.m_sphere_trees[col.category]->update_entity(part_id, pos.value, r.r);
		});

		auto boundaries = ecs.query()
			.notIn<rynx::components::projectile>()
			.in<rynx::components::physical_body, rynx::components::motion, rynx::components::radius, rynx::components::position, rynx::components::boundary, tracked_by_collisions>()
			.for_each_parallel(task_context, [&detection](
				rynx::ecs::id id,
				rynx::components::position pos,
				rynx::components::radius r,
				rynx::components::collisions col)
		{
			rynx_assert((id.value & rynx::collision_detection::mask_id) == id.value, "id out of bounds");
			uint64_t part_id = id.value | mask_kind_boundary;
			detection.m_sphere_trees[col.category]->update_entity(part_id, pos.value, r.r);
		});

		auto projectiles = ecs.query()
			.in<rynx::components::physical_body, rynx::components::projectile, tracked_by_collisions>()
			.for_each_parallel(task_context, [&detection, dt](
				rynx::ecs::id id,
				rynx::components::collisions col,
				rynx::components::position pos,
				rynx::components::motion motion,
				rynx::components::radius r)
		{
			rynx_assert((id.value & rynx::collision_detection::mask_id) == id.value, "id out of bounds");
			uint64_t part_id = id.value | mask_kind_projectile;
			detection.m_sphere_trees[col.category]->update_entity(part_id, pos.value - motion.velocity * dt, r.r);
		});
	});

	task_context.completion_blocked_by(*update_task);
}

void rynx::collision_detection::erase(rynx::ecs::view<const rynx::components::boundary, const rynx::components::projectile> ecs, uint64_t entityId, category_id from) {
	auto& sphere_tree = m_sphere_trees[from.value];
	
	auto entity = ecs[entityId];
	if (const auto* boundary = entity.try_get<const rynx::components::boundary>()) {
		/*
		for (uint32_t i = 0; i < boundary->segments_world.size(); ++i) {
			uint64_t id = mask_kind_boundary | entityId | (uint64_t(i) << bits_id);
			rynx_assert(sphere_tree->contains(id), "");
			sphere_tree->eraseEntity(id);
		}
		*/
		uint64_t id = mask_kind_boundary | entityId;
		rynx_assert(sphere_tree->contains(id), "");
		sphere_tree->eraseEntity(id);
	}
	else if (entity.has<rynx::components::projectile>()) {
		uint64_t id = mask_kind_projectile | entityId;
		rynx_assert(sphere_tree->contains(id), "");
		sphere_tree->eraseEntity(id);
	}
	else {
		uint64_t id = mask_kind_sphere | entityId;
		rynx_assert(sphere_tree->contains(id), "");
		sphere_tree->eraseEntity(id);
	}
}

const rynx::sphere_tree* rynx::collision_detection::get(category_id category) const { return m_sphere_trees[category.value].get(); }