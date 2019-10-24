#pragma once

#include <rynx/tech/object_storage.hpp>
#include <rynx/tech/sphere_tree.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/dynamic_bitset.hpp>
#include <vector>

namespace rynx {
	class collision_detection {
	private:
		enum class check_type {
			both_are_dynamic,
			b_is_static
		};

		struct collision_check {
			collision_check() = default;
			collision_check(sphere_tree* a, sphere_tree* b, check_type type = check_type::both_are_dynamic) : a(a), b(b), type(type) {}
			sphere_tree* a;
			sphere_tree* b;
			check_type type;
		};

		std::vector<std::unique_ptr<sphere_tree>> m_sphere_trees;
		std::vector<collision_check> m_collision_checks;
		dynamic_bitset m_collisionIndex;

	public:
		struct category_id {
			category_id(const category_id&) = default;
			category_id(int32_t value) : value(value) {}
			category_id ignore_collisions() const {
				category_id v(value);
				v.m_ignore_collisions = true;
				return v;
			};

			int32_t value;
			bool m_ignore_collisions = false;
		};

		enum class shape_type {
			Sphere,
			Boundary,
			Projectile
		};

		category_id add_category() {
			m_sphere_trees.emplace_back(std::make_unique<sphere_tree>());
			return category_id(int32_t(m_sphere_trees.size()) - 1);
		}
		
		collision_detection& enable_collisions_between(category_id category1, category_id category2) {
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
		
		template<typename F> void inRange(category_id category, vec3<float> point, float radius, F&& f) {
			m_sphere_trees[category.value]->inRange(pos, radius std::forward<F>(f));
		}

		void update() {
			for (auto& tree : m_sphere_trees) {
				tree->update();
			}
		}

		void update_parallel(rynx::scheduler::task& task_context) {
			for (auto& tree : m_sphere_trees) {
				tree->update_parallel(task_context);
			}
		}

		void erase(uint64_t entityId, category_id from) {
			m_sphere_trees[from.value]->eraseEntity(entityId);
		}

		sphere_tree* get(category_id category) { return m_sphere_trees[category.value].get(); }

		template<typename F> void collisions_for(category_id category, F&& f) {
			sphere_tree::collisions_internal(std::forward<F>(f), &m_sphere_trees[category]->root);
		}

		template<typename F> void collisions_for(category_id category1, category_id category2, F&& f) {
			sphere_tree::collisions_internal(std::forward<F>(f), &m_sphere_trees[category1.value]->root, &m_sphere_trees[category2.value]->root);
		}

		template<typename F> void for_each_collision(F&& f) {
			for (auto&& check : m_collision_checks) {
				if (check.a == check.b)
					sphere_tree::collisions_internal(std::forward<F>(f), &check.a->root);
				else
					sphere_tree::collisions_internal(std::forward<F>(f), &check.a->root, &check.b->root);
			}
		}

		template<typename T, typename F> void for_each_collision_parallel(std::shared_ptr<rynx::parallel_accumulator<T>>& accumulator, F&& f, rynx::scheduler::task& task) {
			for (auto&& check : m_collision_checks) {
				if (check.a == check.b)
					sphere_tree::collisions_internal_parallel(accumulator, std::forward<F>(f), task, &check.a->root);
				else {
					sphere_tree::collisions_internal_parallel_node_node(accumulator, std::forward<F>(f), task, &check.a->root, &check.b->root);
				}
			}
		}
	};
}
