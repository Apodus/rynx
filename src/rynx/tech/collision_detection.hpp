#pragma once

#include <rynx/tech/dynamic_bitset.hpp>
#include <rynx/tech/math/vector.hpp>
#include <vector>
#include <memory>

namespace rynx {
	class sphere_tree;
	template <typename... Ts> class parallel_accumulator;
	
	namespace scheduler {
		class task;
	}
	
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
			}
			
			int32_t value;
			bool m_ignore_collisions = false;
		};

		enum class shape_type {
			Sphere,
			Boundary,
			Projectile
		};

		category_id add_category();
		collision_detection& enable_collisions_between(category_id category1, category_id category2);
		
		template<typename F> void in_radius(category_id category, vec3<float> point, float radius, F&& f) {
			m_sphere_trees[category.value]->in_radius(point, radius, std::forward<F>(f));
		}

		void update();
		void update_parallel(rynx::scheduler::task& task_context);

		void erase(uint64_t entityId, category_id from);
		sphere_tree* get(category_id category);

		template<typename F> void collisions_for(category_id category, F&& f) {
			sphere_tree::collisions_internal(std::forward<F>(f), &m_sphere_trees[category.value]->root);
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
