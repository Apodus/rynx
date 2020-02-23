#include <rynx/tech/collision_detection.hpp>
#include <rynx/tech/sphere_tree.hpp>

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

void rynx::collision_detection::erase(uint64_t entityId, category_id from) {
	m_sphere_trees[from.value]->eraseEntity(entityId);
}

rynx::sphere_tree* rynx::collision_detection::get(category_id category) { return m_sphere_trees[category.value].get(); }