#pragma once

#include <rynx/tech/dynamic_bitset.hpp>
#include <rynx/tech/sphere_tree.hpp>
#include <rynx/tech/components.hpp>
#include <rynx/math/vector.hpp>
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
		struct tracked_by_collisions {};

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
		
	public:
		struct category_id {
			category_id() = default;
			category_id(const category_id&) = default;
			category_id(int32_t value) : value(value) {}
			category_id ignore_collisions() const {
				category_id v(value);
				v.m_ignore_collisions = true;
				return v;
			}
			
			int32_t value = -1;
			bool m_ignore_collisions = false;
		};

		enum kind : uint64_t {
			sphere = 1,
			boundary2d = 2,
			projectile = 3
		};

		struct collision_params {
			uint64_t id1;
			uint64_t id2;
			uint32_t kind1;
			uint32_t kind2;
			uint32_t part1;
			uint32_t part2;
			float radius1;
			float radius2;
			vec3f pos1;
			vec3f pos2;
			vec3f normal;
			float penetration;
		};

		static constexpr uint32_t bits_id = rynx::ecs_id_bits;
		static constexpr uint32_t bits_part = 20;
		static constexpr uint32_t bits_kind = 2;

		static constexpr uint64_t mask_id = (uint64_t(1) << bits_id) - 1;
		static constexpr uint64_t mask_part = ~mask_id & (~uint64_t(0) >> bits_kind);

		static constexpr uint32_t bitshift_part = bits_id;
		static constexpr uint32_t bitshift_kind = bits_id + bits_part;

		static constexpr uint64_t mask_kind_sphere = kind::sphere << bitshift_kind;
		static constexpr uint64_t mask_kind_boundary = kind::boundary2d << bitshift_kind;
		static constexpr uint64_t mask_kind_projectile = kind::projectile << bitshift_kind;

		static_assert(bits_id + bits_part + bits_kind <= sizeof(mask_id) * 8);
		
		category_id add_category();
		collision_detection& enable_collisions_between(category_id category1, category_id category2);
		
		template<typename F> void in_radius(category_id category, vec3<float> point, float radius, F&& f) {
			m_sphere_trees[category.value]->in_radius(point, radius, std::forward<F>(f));
		}

		void clear();
		void update_sphere_trees();
		void update_sphere_trees_parallel(rynx::scheduler::task& task_context);
		
		void track_entities(rynx::scheduler::task& task_context);
		void update_entities(rynx::scheduler::task& task_context, float dt);
		
		// api for slow operations that are only supposed to be used from an editor context
		struct editor_api_t {
			editor_api_t(rynx::collision_detection* host) : m_host(host) {}
			
			void update_entity_forced(rynx::ecs& ecs, rynx::ecs::id id);
			void remove_collision_from_entity(rynx::ecs::edit_view<const tracked_by_collisions, const rynx::components::collisions> ecs, rynx::ecs::id id);
			void set_collision_category_for_entity(rynx::ecs& ecs, rynx::ecs::id id, category_id collision_category);

		private:
			rynx::collision_detection* m_host;
		};

		editor_api_t editor_api() { return editor_api_t(this); }
		

		void erase(rynx::ecs::view<const rynx::components::boundary, const rynx::components::projectile> ecs, uint64_t entityId, category_id from);
		const sphere_tree* get(category_id category) const;

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
			using storage_t = decltype(accumulator->template get_local_storage<T>());
			for (auto&& check : m_collision_checks) {
				if (check.a == check.b)
					sphere_tree::collisions_internal_parallel(accumulator, [f](storage_t& storage, uint64_t id1, uint64_t id2, vec3f pos1, float radius1, vec3f pos2, float radius2, vec3f normal, float penetration) {
						collision_params params;
						params.id1 = id1 & mask_id;
						params.id2 = id2 & mask_id;
						params.kind1 = id1 >> bitshift_kind;
						params.kind2 = id2 >> bitshift_kind;
						params.normal = normal;
						params.part1 = (id1 & mask_part) >> bitshift_part;
						params.part2 = (id2 & mask_part) >> bitshift_part;
						rynx_assert(params.part1 == 0 && params.part2 == 0, "o ou");
						params.penetration = penetration;
						params.pos1 = pos1;
						params.pos2 = pos2;
						params.radius1 = radius1;
						params.radius2 = radius2;
						f(storage, params);
					}, task, &check.a->root);
				else {
					sphere_tree::collisions_internal_parallel_node_node(accumulator, [f](storage_t& storage, uint64_t id1, uint64_t id2, vec3f pos1, float radius1, vec3f pos2, float radius2, vec3f normal, float penetration) {
						collision_params params;
						params.id1 = id1 & mask_id;
						params.id2 = id2 & mask_id;
						params.kind1 = id1 >> bitshift_kind;
						params.kind2 = id2 >> bitshift_kind;
						params.normal = normal;
						params.part1 = (id1 & mask_part) >> bitshift_part;
						params.part2 = (id2 & mask_part) >> bitshift_part;
						rynx_assert(params.part1 == 0 && params.part2 == 0, "o ou");
						params.penetration = penetration;
						params.pos1 = pos1;
						params.pos2 = pos2;
						params.radius1 = radius1;
						params.radius2 = radius2;
						f(storage, params);
					}, task, &check.a->root, &check.b->root);
				}
			}
		}
	};
}
