#pragma once

#include <rynx/tech/object_storage.hpp>
#include <rynx/tech/sphere_tree.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/dynamic_bitset.hpp>
#include <vector>

namespace rynx {
	class collision_detection {
	private:
		struct collision_check {
			collision_check() = default;
			collision_check(sphere_tree* a, sphere_tree* b) : a(a), b(b) {}
			sphere_tree* a;
			sphere_tree* b;
		};

		std::vector<std::unique_ptr<sphere_tree>> m_sphere_trees;
		std::vector<collision_check> m_collision_checks;
		dynamic_bitset m_collisionIndex;

	public:
		struct category_id {
			category_id(int32_t value) : value(value) {}
			int32_t value;
		};

		enum class shape_type {
			Sphere,
			Boundary,
			Projectile
		};

		category_id addCategory() { m_sphere_trees.emplace_back(std::make_unique<sphere_tree>()); return category_id(int32_t(m_sphere_trees.size()) - 1); }
		collision_detection& enableCollisionsBetweenCategories(category_id category1, category_id category2) { m_collision_checks.emplace_back(m_sphere_trees[category1.value].get(), m_sphere_trees[category2.value].get()); return *this; }

		template<typename F> void inRange(category_id category, vec3<float> point, float radius, F&& f) {
			m_sphere_trees[category.value]->inRange(pos, radius std::forward<F>(f));
		}

		void update() {
			for (auto& tree : m_sphere_trees) {
				tree->update();
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

		// TODO: figure out if this is a useful abstraction to have or not :(
		dynamic_bitset& collidedThisFrame() { return m_collisionIndex; }
	};
}
