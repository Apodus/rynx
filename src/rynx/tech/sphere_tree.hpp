#pragma once

#include <rynx/tech/math/vector.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/math/geometry.hpp>
#include <rynx/tech/profiling.hpp>
#include <vector>

inline float sqr(float x) { return x * x; };

namespace rynx {
	class collision_detection;
}

// for parallel collision detection
// TODO: get rid of this include. move functionality to some other file?
#include <rynx/scheduler/task.hpp>

class sphere_tree {
	friend class rynx::collision_detection;
public:
	using index_t = uint32_t;

private:
	static constexpr int MaxElementsInNode = 128; // this is an arbitrary constant. might be smarter ways to pick it.
	static constexpr int MaxNodesInNode = MaxElementsInNode >> 1;
	
	struct entry {
		entry() = default;
		entry(vec3<float> p, float r, uint64_t i) : pos(p), radius(r), entityId(i) {}

		vec3<float> pos;
		float radius = 0;
		uint64_t entityId;
	};

	struct node {
		node(vec3<float> pos, node* parent, int depth = 0) : pos(pos), m_parent(parent), depth(depth) {}

		vec3<float> pos;
		float radius = 0;
		int32_t depth = 0;
		int32_t parent_optimization_interleave = 0; // [0, 1] - each layer switches which children are updated

		void killMePlease(node* child) {
			// TODO: nodes could have this index stored. is not hard.
			for (size_t i = 0; i < m_children.size(); ++i) {
				if (m_children[i].get() == child) {
					killMePlease(int(i));
					return;
				}
			}

			rynx_assert(false, "must find the kill");
		}

		void killMePlease(int index) {
			m_children[index] = std::move(m_children.back());
			m_children.pop_back();

			// if 1 child left, add child to parent. and kill self.
			if (m_children.size() == 1) {
				if (m_parent) {
					m_children[0]->m_parent = m_parent;
					m_parent->m_children.emplace_back(std::move(m_children[0]));
					m_parent->killMePlease(this);
				}
			}
		}

		uint64_t entity_migrates(index_t member_index, sphere_tree* container) {
			auto id_of_erased = m_members[member_index].entityId;
			m_members[member_index] = std::move(m_members.back());
			container->entryMap.find(m_members[member_index].entityId)->second.second = member_index;
			m_members.pop_back();

			/*
			if (m_members.empty() && m_parent != nullptr) {
				m_parent->killMePlease(this);
			}
			*/

			// TODO: check, if me and siblings are too many in number vs. the number of elements stored,
			// merge some of the siblings?
			return id_of_erased;
		}

		std::unique_ptr<node> node_migrates(node* child_ptr) {
			for (size_t i = 0; i < m_children.size(); ++i) {
				if (m_children[i].get() == child_ptr) {
					std::unique_ptr<node> child = std::move(m_children[i]);
					m_children[i] = std::move(m_children.back());
					m_children.pop_back();
					return child;
				}
			}
			rynx_assert(false, "unreachable");
		}


		void erase(index_t member_index, sphere_tree* container) {
			uint64_t erasedEntity = entity_migrates(member_index, container);
			container->entryMap.erase(erasedEntity);
		}

		void insert(entry&& item, sphere_tree* container) {
			rynx_assert(m_children.empty(), "insert can only be done on leaf nodes");
			m_members.emplace_back(std::move(item));
			container->entryMap.insert_or_assign(m_members.back().entityId, std::pair<node*, index_t>(this, index_t(m_members.size() - 1)));

			if (m_members.size() >= MaxElementsInNode) {
				// if we have no parent (we are root node), add a couple of new child nodes under me.
				if (!m_parent) {
					/*
					// split to new child level.
					auto f1 = farPoint(members[0].pos, members);
					auto f2 = farPoint(members[f1.first].pos, members);

					children.emplace_back(std::make_unique<node>(members[f1.first].pos, this, depth + 1));
					children.emplace_back(std::make_unique<node>(members[f2.first].pos, this, depth + 1));

					for (auto&& member : members) {
						int index = (member.pos - members[f1.first].pos).lengthSquared() < (member.pos - members[f2.first].pos).lengthSquared();
						children[index]->insert(std::move(member), container);
						// child insert takes care of update calls.
					}
					members.clear();
					*/

					for (size_t i = 0; i < MaxNodesInNode; ++i) {
						m_children.emplace_back(std::make_unique<node>(m_members[i].pos, this, depth + 1));
					}
					for (size_t i = 0; i < m_members.size(); ++i) {
						m_children[i % m_children.size()]->insert(std::move(m_members[i]), container);
					}
					m_members.clear();

					for (auto& child : m_children) {
						child->update_single();
					}
				}
				else {
					// if also our parent node is full, and can't have new children.
					// add a new layer of nodes between my parent, and myself.
					if (m_parent->m_children.size() >= MaxNodesInNode) {
						// first take the existing children to safety.
						std::vector<std::unique_ptr<node>> children_of_parent = std::move(m_parent->m_children);

#if 1
						m_parent->m_children.clear();
						for (size_t i = 0; i < MaxNodesInNode; ++i) {
							m_parent->m_children.emplace_back(std::make_unique<node>(children_of_parent[i]->pos, m_parent, m_parent->depth + 1));
						}

						node* old_parent = m_parent;
						for (size_t i = 0; i < children_of_parent.size(); ++i) {
							auto& old_child = children_of_parent[i];
							auto& new_parent = old_parent->m_children[i % old_parent->m_children.size()];
							old_child->m_parent = new_parent.get();
							new_parent->m_children.emplace_back(std::move(old_child));
						}
						
						children_of_parent.clear();
#else
						// clear the state of parent's children structure, and add three new nodes there.
						parent->children.clear();
						parent->children.emplace_back(std::make_unique<node>(children_of_parent[0]->pos, parent, parent->depth + 1));
						parent->children.emplace_back(std::make_unique<node>(children_of_parent[1]->pos, parent, parent->depth + 1));
						parent->children.emplace_back(std::make_unique<node>(children_of_parent[2]->pos, parent, parent->depth + 1));

						node* old_parent = parent;

						auto node0_pos = children_of_parent[0]->pos;
						auto node1_pos = children_of_parent[1]->pos;
						auto node2_pos = children_of_parent[2]->pos;

						// assign children to closest new parent nodes.
						for (size_t i = 0; i < children_of_parent.size(); ++i) {
							auto& old_child = children_of_parent[i];

							float dist0 = (old_child->pos - node0_pos).lengthSquared();
							float dist1 = (old_child->pos - node1_pos).lengthSquared();
							float dist2 = (old_child->pos - node2_pos).lengthSquared();

							if (dist0 < dist1) {
								if (dist0 < dist2) {
									old_child->parent = old_parent->children[0].get();
									old_parent->children[0]->children.emplace_back(std::move(old_child));
								}
								else {
									old_child->parent = old_parent->children[2].get();
									old_parent->children[2]->children.emplace_back(std::move(old_child));
								}
							}
							else {
								if (dist1 < dist2) {
									old_child->parent = old_parent->children[1].get();
									old_parent->children[1]->children.emplace_back(std::move(old_child));
								}
								else {
									old_child->parent = old_parent->children[2].get();
									old_parent->children[2]->children.emplace_back(std::move(old_child));
								}
							}
						}
						children_of_parent.clear();
#endif
						for (auto& child : old_parent->m_children) {
							child->refresh_depths();
						}
					}

					// now there should be space in parent node's children list, add new siblings to self there.
					auto f1 = farPoint(m_members.back().pos, m_members);
					{
						m_parent->m_children.emplace_back(std::make_unique<node>(m_members[f1.first].pos, m_parent, depth));
						auto id = m_members[f1.first].entityId;
						m_parent->m_children.back()->m_members.emplace_back(std::move(m_members[f1.first]));

						auto& datap = container->entryMap.find(id)->second;
						datap.first = m_parent->m_children.back().get();
						datap.second = 0; // update mapping of the moved child!
					}

					if (f1.first != m_members.size() - 1) {
						auto id = m_members.back().entityId;
						m_members[f1.first] = std::move(m_members.back());
						container->entryMap.find(id)->second.second = index_t(f1.first); // also need to update mapping of new f1.first
					}

					m_members.pop_back();

					// these probably should not be called all the time. only after all move ops are done.
					m_parent->m_children.back()->update_single();
				}
			}
		}

		void refresh_depths() {
			for (auto& child : m_children) {
				child->depth = depth + 1;
				child->refresh_depths();
			}
		}

		void remove_empty_nodes() {
			for (size_t i = 0; i < m_children.size(); ++i) {
				auto& child = m_children[i];
				if (child->m_children.empty() & child->m_members.empty()) {
					killMePlease(int(i));
					--i;
				}
				else {
					child->remove_empty_nodes();
				}
			}
		}

		void update_from_leaf_to_root() {
			for (size_t i = 0; i < m_children.size(); ++i) {
				auto& child = m_children[i];
				child->update_from_leaf_to_root();
			}
			update_single();
		}

		void update_single() {
			std::pair<vec3<float>, float> posInfo;
			if (m_children.empty()) {
				if (m_members.empty()) {
					return;
				}
				posInfo = bounding_sphere(m_members);
			}
			else {
				posInfo = bounding_sphere(m_children);
			}

			pos = posInfo.first;
			radius = posInfo.second;
		}

		// TODO: Optimization: This can be parallelized.
		void find_optimized_parents_for_nodes() {
			if (m_parent) {
				auto new_parent = root()->find_nearest_parent(pos, depth, std::numeric_limits<float>::max());
				if (
					new_parent.first &&
					new_parent.first != m_parent
					)
				{
					m_new_parent = new_parent.first;
				}
			}

			parent_optimization_interleave ^= 1;
			for (size_t i = parent_optimization_interleave; i < m_children.size(); i += 2) {
				m_children[i]->find_optimized_parents_for_nodes();
			}
		}

		void apply_optimized_parents_for_nodes() {
			if (m_new_parent) {
				std::unique_ptr<node> myself = m_parent->node_migrates(this);
				myself->m_parent = m_new_parent;
				m_new_parent->m_children.emplace_back(std::move(myself));
				m_new_parent->update_single();
				m_new_parent = nullptr;
			}

			// this mutates the m_children vector and thus cannot be range for, or iterators.
			for (size_t i = 0; i < m_children.size(); ++i) {
				m_children[i]->apply_optimized_parents_for_nodes();
			}
		}

		node* root() {
			if (!m_parent)
				return this;
			
			node* next = m_parent;
			while (true) {
				if (!next->m_parent)
					return next;
				next = next->m_parent;
			}
		}

		std::pair<node*, float> find_nearest_parent(vec3<float> point, int source_depth, float maxDistSqr) const {
			std::pair<node*, float> ans(nullptr, maxDistSqr);
			for (const auto& child : m_children) {
				if (!child->m_members.empty())
					continue;
				
				float potentialSqrDist = (child->pos - point).lengthSquared() - child->radius * child->radius; // TODO: This is not true.
				if (potentialSqrDist < ans.second) {
					if (source_depth > child->depth) {
						if (source_depth == child->depth + 1) {
							std::pair<node*, float> potentialAns = {child.get(), (child->pos - point).lengthSquared()};
							if (potentialAns.second < ans.second) {
								ans = potentialAns;
							}
						}
						else {
							if (!child->m_children.empty()) {
								auto potentialAns = child->find_nearest_parent(point, source_depth, ans.second);
								if (potentialAns.second < ans.second) {
									ans = potentialAns;
								}
							}
						}
					}
				}
			}

			return ans;
		}

		template<typename F> void forEachNode(F&& f, int depth = 0) {
			f(pos, radius, depth);
			for (auto& child : m_children) {
				child->forEachNode(std::forward<F>(f), depth + 1);
			}
		}

		template<typename F>
		void inRange(vec3<float> point, float range, F&& f) {
			if ((pos - point).lengthSquared() < sqr(radius + range)) {
				if (m_children.empty()) {
					for (auto&& item : m_members) {
						if ((item.pos - point).lengthSquared() < range * range) {
							f(item.entityId, item.pos, item.radius);
						}
					}
				}
				else {
					for (auto&& child : m_children) {
						child->inRange(point, range, std::forward<F>(f));
					}
				}
			}
		}

		std::pair<node*, float> findNearestLeaf(vec3<float> point, float maxDistSqr) {
			if (m_children.empty()) {
				return { this, (point - pos).lengthSquared() };
			}

			std::pair<node*, float> ans(nullptr, maxDistSqr);
			for (auto&& child : m_children) {
				float potentialSqrDist = (child->pos - point).lengthSquared() - child->radius * child->radius;
				if (potentialSqrDist < ans.second) {
					auto potentialAns = child->findNearestLeaf(point, ans.second);
					if(potentialAns.second < ans.second) {
						ans = potentialAns;
					}
				}
			}

			return ans;
		}

		node* m_parent = nullptr;
		node* m_new_parent = nullptr;
		std::vector<std::unique_ptr<node>> m_children; // child nodes
		std::vector<entry> m_members;
	};

	rynx::unordered_map<uint64_t, std::pair<node*, index_t>> entryMap;
	node root;
	size_t update_next_index = 0;
	uint64_t update_iteration_counter = 0;

public:
	sphere_tree() : root({0, 0, 0}, nullptr) {}

	void updateEntity(vec3<float> pos, float radius, uint64_t entityId) {
		auto it = entryMap.find(entityId);
		if (it != entryMap.end()) {
			auto& data = it->second.first->m_members[it->second.second];
			data.pos = pos;
			data.radius = radius;
		}
		else {
			auto res = root.findNearestLeaf(pos, std::numeric_limits<float>::max());
			res.first->insert(entry(pos, radius, entityId), this);
		}
	}

	void eraseEntity(uint64_t entityId) {
		auto it = entryMap.find(entityId);
		if (it != entryMap.end()) {
			uint64_t id_of_erased = it->second.first->entity_migrates(it->second.second, this);
			rynx_assert(id_of_erased == entityId, "mismatch of erased entity vs expected");
			entryMap.erase(it);
		}
	}

	void update() {
		{
			rynx_profile("SphereTree", "FindBuckets");
			if (entryMap.empty())
				return;

			auto it = entryMap.iteratorAt(update_next_index);
			
			// NOTE: we are updating 1/32 of all entries per iteration. this is because the update itself is really fucking expensive.
			//       this update optimizes the parent node of the entities. skipping the update means that our sphere tree bottom level will be slightly
			//       unoptimized, which results in more work during collision- and inRange queries. But because changes in entry positions are seldom radical, this is usually perfectly fine.
			for (size_t i = 0; i <= (entryMap.size() >> 5) + 1; ++i, ++it) {
				if (it == entryMap.end()) {
					it = entryMap.begin();
				}

				auto item = it->second.first->m_members[it->second.second];
				auto newLeaf = root.findNearestLeaf(item.pos, (it->second.first->pos - item.pos).lengthSquared() * 1.001f);
				if (!newLeaf.first || newLeaf.first == it->second.first) {
					continue;
				}

				// move to new bucket.
				it->second.first->entity_migrates(it->second.second, this);
				newLeaf.first->insert(std::move(item), this);
			}

			update_next_index = it.index();
		}

		{
			rynx_profile("SphereTree", "Find optimized parents for nodes");
			root.find_optimized_parents_for_nodes();
		}

		{
			rynx_profile("SphereTree", "Update parents for nodes");
			root.apply_optimized_parents_for_nodes();
		}

		{
			rynx_profile("SphereTree", "Remove empty nodes");
			root.remove_empty_nodes();
		}

		{
			rynx_profile("SphereTree", "Update node positions");
			std::vector<std::vector<node*>> node_levels(1, { &root });
			for(;;)
			{
				std::vector<node*> next_level;
				for (const node* node_ptr : node_levels.back()) {
					for (auto& child_ptr : node_ptr->m_children) {
						next_level.emplace_back(child_ptr.get());
					}
				}
				if (next_level.empty())
					break;
				node_levels.emplace_back(std::move(next_level));
			}

			for (auto it = node_levels.rbegin(); it != node_levels.rend(); ++it) {
				for (auto* node : *it) {
					node->update_single();
				}
			}
		}
	}

	void update_parallel(rynx::scheduler::task& task_context) {
		
		if (entryMap.empty())
			return;

		// NOTE: we are updating 1/16 of all entries per iteration. this is because the update itself is quite expensive.
		//       this update optimizes the parent node of the entities. skipping the update means that our sphere tree bottom level will be slightly
		//       unoptimized, which results in more work during collision- and inRange queries. But because changes in entry positions are seldom radical,
		//       updating some smallish time duration later is usually perfectly fine.
		auto capacity = entryMap.capacity();
		rynx_assert((capacity & (capacity - 1)) == 0, "must be power of 2");

		struct plip {
			int64_t map_index;
			node* new_parent;
		};

		std::shared_ptr<std::mutex> entity_migrate_mutex = std::make_shared<std::mutex>();
		std::shared_ptr<std::vector<plip>> found_migrates = std::make_shared<std::vector<plip>>();
		rynx::scheduler::barrier entities_migrated_bar;

		auto limit = (capacity >> 4) + 1;
		auto bar = task_context.parallel().for_each(update_next_index, update_next_index + limit, [this, capacity, m = std::move(entity_migrate_mutex), found_migrates](int64_t index) {
			index &= capacity - 1;
			if (entryMap.slot_test(index)) {
				auto entry = entryMap.slot_get(index);
				auto item = entry.second.first->m_members[entry.second.second];
				auto newLeaf = root.findNearestLeaf(item.pos, (entry.second.first->pos - item.pos).lengthSquared() * 1.001f);

				// move to new bucket. the lock is kind of annoying, but on the other hand.
				// there are very few entities migrating per frame. so it should not be terrible, maybe? TODO: Measure.
				if (newLeaf.first && newLeaf.first != entry.second.first) {
					std::lock_guard lock(*m);
					found_migrates->emplace_back(plip{ index, newLeaf.first });
				}
			}
		});

		task_context.extend_task_execute_parallel([this, found_migrates]() {
			for (auto&& migratee : *found_migrates) {
				auto entry = entryMap.slot_get(migratee.map_index);
				auto item = entry.second.first->m_members[entry.second.second];

				entry.second.first->entity_migrates(entry.second.second, this);
				migratee.new_parent->insert(std::move(item), this);
			}
		})->depends_on(bar).required_for(entities_migrated_bar);

		update_next_index += limit;
		update_next_index &= capacity - 1;
		

		task_context.extend_task_execute_parallel([this](rynx::scheduler::task& task_context) {
			{
				rynx_profile("SphereTree", "Find optimized parents for nodes");
				root.find_optimized_parents_for_nodes();
			}

			{
				rynx_profile("SphereTree", "Update parents for nodes");
				root.apply_optimized_parents_for_nodes();
			}

			{
				rynx_profile("SphereTree", "Remove empty nodes");
				root.remove_empty_nodes();
			}

			{
				rynx_profile("SphereTree", "Update node positions");
				std::vector<std::vector<node*>> node_levels(1, { &root });
				std::vector<node*> flat_answer;
				for (;;)
				{
					flat_answer.insert(flat_answer.end(), node_levels.back().begin(), node_levels.back().end());

					std::vector<node*> next_level;
					for (const node* node_ptr : node_levels.back()) {
						for (auto& child_ptr : node_ptr->m_children) {
							next_level.emplace_back(child_ptr.get());
						}
					}
					if (next_level.empty())
						break;
					node_levels.emplace_back(std::move(next_level));
				}

				// doing this in one parallel_for is not 100% correct, because at layer boundaries should sync.
				// if we start computing on second layer before first has completed, it is possible that a collision between
				// objects is detected one frame late. but this does give 60% performance boost, and the error case of possible late detection isn't too bad.
				std::reverse(flat_answer.begin(), flat_answer.end());
				size_t s = flat_answer.size();
				task_context.parallel().for_each(0, s, [layer = std::move(flat_answer)](int64_t i) {
					layer[i]->update_single();
				}, 8);

				// This would give 100% correct result in all situations. But it has to sync between layers, which is slow.
				/*
				// update node positions & radii from leaf to root. sync between layers.
				for (auto it = node_levels.rbegin(); it != node_levels.rend(); ++it) {
					task_context.extend_task_execute_sequential("update node pos & r", [this, layer = std::move(*it)](rynx::scheduler::task& task_context) {
						size_t size = layer.size();
						task_context.parallel().for_each(0, size, [layer = std::move(layer)](int64_t i) {
							layer[i]->update_single();
						}, 8);
					});
				}
				*/
			}
		})->depends_on(entities_migrated_bar);
	}

	template<typename F>
	void inRange(vec3<float> pos, float radius, F&& f) {
		root.inRange(pos, radius, std::forward<F>(f));
	}

	template<typename F>
	void collisions(F&& f) {
		collisions_internal(std::forward<F>(f), &root);
	}

	template<typename F>
	void forEachNode(F&& f) {
		root.forEachNode(std::forward<F>(f));
	}

private:
	template<typename F> static void collisions_internal_parallel_intra_node(F&& f, rynx::scheduler::task& task, const node* rynx_restrict a) {
		rynx_assert(a != nullptr, "node cannot be null");

		std::vector<const node*> prev_layer(1, { a });
		std::vector<const node*> leaf_nodes;
		if(a->m_children.empty())
			leaf_nodes.emplace_back(a);

		for (;;)
		{
			std::vector<const node*> next_layer;
			for (const node* node_ptr : prev_layer) {
				for (auto& child_ptr : node_ptr->m_children) {
					if (child_ptr->m_children.empty())
						leaf_nodes.emplace_back(child_ptr.get());
					else
						next_layer.emplace_back(child_ptr.get());
				}
			}
			if (next_layer.empty())
				break;
			prev_layer = std::move(next_layer);
		}

		auto leaf_node_count = leaf_nodes.size();
		task.parallel().for_each(0, leaf_node_count, [f, leaf_nodes = std::move(leaf_nodes)](int64_t node_index) {
			const node* a = leaf_nodes[node_index];
			for (size_t i = 0; i < a->m_members.size(); ++i) {
				const auto& m1 = a->m_members[i];
				for (size_t k = i + 1; k < a->m_members.size(); ++k) {
					const auto& m2 = a->m_members[k];
					float distSqr = (m1.pos - m2.pos).lengthSquared();
					float radiusSqr = sqr(m1.radius + m2.radius);
					if (distSqr < radiusSqr) {
						f(m1.entityId, m2.entityId, (m1.pos - m2.pos).normalizeApprox(), std::sqrtf(radiusSqr) - std::sqrtf(distSqr));
					}
				}
			}
		}, 8);
	}

	static void collisions_internal_gather_leaf_node_pairs(
		const node* rynx_restrict a,
		const node* rynx_restrict b,
		rynx::unordered_map<const node*, std::vector<const node*>>& leaf_pairs)
	{
		rynx_assert(a != nullptr, "node cannot be null");
		rynx_assert(b != nullptr, "node cannot be null");
		rynx_assert(a != b, "nodes must differ");

		if ((b->pos - a->pos).lengthSquared() > sqr(a->radius + b->radius)) {
			return;
		}

		if (a->m_children.empty()) {
			if (b->m_children.empty()) {
				leaf_pairs.emplace(a, std::vector<const node*>()).first->second.emplace_back(b);
				rynx_assert(leaf_pairs[a].size() > 0, "???");
				rynx_assert(leaf_pairs.slot_test(leaf_pairs.find(a).index()), "??");
				rynx_assert(leaf_pairs.slot_get(leaf_pairs.find(a).index()).second.size() > 0, "??");
			}
			else {
				for (const auto& child : b->m_children) {
					collisions_internal_gather_leaf_node_pairs(a, child.get(), leaf_pairs);
				}
			}
		}
		else {
			if (b->m_children.empty()) {
				for (const auto& child : a->m_children) {
					collisions_internal_gather_leaf_node_pairs(child.get(), b, leaf_pairs);
				}
			}
			else if (a->radius > b->radius) {
				for (const auto& child : a->m_children) {
					collisions_internal_gather_leaf_node_pairs(child.get(), b, leaf_pairs);
				}
			}
			else {
				for (const auto& child : b->m_children) {
					collisions_internal_gather_leaf_node_pairs(a, child.get(), leaf_pairs);
				}
			}
		}
	}

	// This is only safe to be called when node* b contains only static entities (will not store collisions)
	template<typename F> static void collisions_internal_parallel_b_static(
		F&& f,
		rynx::scheduler::task& task_context,
		const node* rynx_restrict a,
		const node* rynx_restrict b)
	{
		std::shared_ptr<rynx::unordered_map<const node*, std::vector<const node*>>> leaf_pairs = std::make_shared<rynx::unordered_map<const node*, std::vector<const node*>>>();
		collisions_internal_gather_leaf_node_pairs(a, b, *leaf_pairs);

		task_context.parallel().for_each(0, leaf_pairs->capacity(), [f, leaf_pairs](int64_t i) {
			if (!leaf_pairs->slot_test(i))
				return;
			
			auto& entry = leaf_pairs->slot_get(i);
			const node* a = entry.first;
			for (const node* b : entry.second) {
				for (const auto& member1 : a->m_members) {
					if ((member1.pos - b->pos).lengthSquared() < sqr(member1.radius + b->radius)) {
						for (const auto& member2 : b->m_members) {
							float distSqr = (member1.pos - member2.pos).lengthSquared();
							float radiusSqr = sqr(member1.radius + member2.radius);
							if (distSqr < radiusSqr) {
								auto normal = (member1.pos - member2.pos).normalizeApprox();
								float penetration = std::sqrtf(radiusSqr) - std::sqrtf(distSqr);
								f(member1.entityId, member2.entityId, normal, penetration);
							}
						}
					}
				}
			}
		}, 1);
	}


	template<typename F> static void collisions_internal_parallel(F&& f, rynx::scheduler::task& task, const node* rynx_restrict a) {
		rynx_assert(a != nullptr, "node cannot be null");
		task.extend_task_execute_sequential("intra-node", [f, a](rynx::scheduler::task& task) {
			collisions_internal_parallel_intra_node(f, task, a);
		});

		std::vector<const node*> current_level;
		current_level.emplace_back(a);

		for (;;) {
			std::vector<const node*> copy_for_task = current_level;
			task.extend_task_execute_sequential("inter-node", [f, current_level = std::move(copy_for_task)](rynx::scheduler::task& task) {
				for (const node* a : current_level) {
					if (a->depth == 0 && a->m_children.size() >= size_t(16)) {
						auto two_ranges = [f, a](size_t begin1, size_t end1, size_t begin2, size_t end2) {
							rynx_assert(end1 <= a->m_children.size(), "iterating past end");
							rynx_assert(end2 <= a->m_children.size(), "iterating past end");
							for (size_t i = begin1; i < end1; ++i) {
								const node* child1 = a->m_children[i].get();
								rynx_assert(child1 != nullptr, "node cannot be null");
								rynx_assert(child1 != a, "nodes must differ");

								for (size_t k = begin2; k < end2; ++k) {
									const node* child2 = a->m_children[k].get();
									rynx_assert(child2 != nullptr, "node cannot be null");
									rynx_assert(child2 != child1, "nodes must differ");

									if ((child1->pos - child2->pos).lengthSquared() < sqr(child1->radius + child2->radius)) {
										collisions_internal(f, child1, child2);
									}
								}
							}
						};

						auto one_range = [f, a](size_t begin1, size_t end1) {
							rynx_assert(end1 <= a->m_children.size(), "iterating past end");
							for (size_t i = begin1; i < end1; ++i) {
								const node* child1 = a->m_children[i].get();
								rynx_assert(child1 != nullptr, "node cannot be null");
								rynx_assert(child1 != a, "nodes must differ");

								for (size_t k = i + 1; k < end1; ++k) {
									const node* child2 = a->m_children[k].get();
									rynx_assert(child2 != nullptr, "node cannot be null");
									rynx_assert(child2 != child1, "nodes must differ");

									if ((child1->pos - child2->pos).lengthSquared() < sqr(child1->radius + child2->radius)) {
										collisions_internal(f, child1, child2);
									}
								}
							}
						};

						int block_size = int(a->m_children.size() >> 3);

						task.extend_task_execute_sequential([one_range, block_size, a](rynx::scheduler::task& task) {
							for (int i = 0; i < 7; ++i) {
								task.extend_task_execute_parallel([one_range, block_size, i]() {
									one_range(i * block_size, (i + 1) * block_size);
								});
							}
							one_range(7 * block_size, a->m_children.size());
						});

						auto call = [two_ranges, block_size, a](int i, int k) {
							int begin1 = i * block_size;
							int end1 = (i + 1) * block_size;
							int begin2 = k * block_size;
							int end2 = (k < 7) ? (k + 1) * block_size : int(a->m_children.size());
							two_ranges(begin1, end1, begin2, end2);
						};
							
						task.extend_task_execute_sequential([call, a](rynx::scheduler::task& task) {
							task.extend_task_execute_parallel([call]() { call(0, 1); });
							task.extend_task_execute_parallel([call]() { call(2, 3); });
							task.extend_task_execute_parallel([call]() { call(4, 5); });
							task.extend_task_execute_parallel([call]() { call(6, 7); });
						});

						task.extend_task_execute_sequential([call, a](rynx::scheduler::task& task) {
							task.extend_task_execute_parallel([call]() { call(0, 2); });
							task.extend_task_execute_parallel([call]() { call(1, 3); });
							task.extend_task_execute_parallel([call]() { call(4, 6); });
							task.extend_task_execute_parallel([call]() { call(5, 7); });
						});

						task.extend_task_execute_sequential([call, a](rynx::scheduler::task& task) {
							task.extend_task_execute_parallel([call]() { call(0, 3); });
							task.extend_task_execute_parallel([call]() { call(1, 2); });
							task.extend_task_execute_parallel([call]() { call(4, 7); });
							task.extend_task_execute_parallel([call]() { call(5, 6); });
						});

						task.extend_task_execute_sequential([call, a](rynx::scheduler::task& task) {
							task.extend_task_execute_parallel([call]() { call(0, 4); });
							task.extend_task_execute_parallel([call]() { call(1, 5); });
							task.extend_task_execute_parallel([call]() { call(2, 6); });
							task.extend_task_execute_parallel([call]() { call(3, 7); });
						});

						task.extend_task_execute_sequential([call, a](rynx::scheduler::task& task) {
							task.extend_task_execute_parallel([call]() { call(0, 5); });
							task.extend_task_execute_parallel([call]() { call(1, 4); });
							task.extend_task_execute_parallel([call]() { call(2, 7); });
							task.extend_task_execute_parallel([call]() { call(3, 6); });
						});

						task.extend_task_execute_sequential([call, a](rynx::scheduler::task& task) {
							task.extend_task_execute_parallel([call]() { call(0, 6); });
							task.extend_task_execute_parallel([call]() { call(1, 7); });
							task.extend_task_execute_parallel([call]() { call(2, 5); });
							task.extend_task_execute_parallel([call]() { call(3, 4); });
						});

						task.extend_task_execute_sequential([call, a](rynx::scheduler::task& task) {
							task.extend_task_execute_parallel([call]() { call(0, 7); });
							task.extend_task_execute_parallel([call]() { call(1, 6); });
							task.extend_task_execute_parallel([call]() { call(2, 4); });
							task.extend_task_execute_parallel([call]() { call(3, 5); });
						});
					}
					else {
						std::vector<std::pair<const node*, const node*>> collision_check_node_pair;
						for (size_t i = 0; i < a->m_children.size(); ++i) {
							const node* child1 = a->m_children[i].get();
							rynx_assert(child1 != nullptr, "node cannot be null");
							rynx_assert(child1 != a, "nodes must differ");

							for (size_t k = i + 1; k < a->m_children.size(); ++k) {
								const node* child2 = a->m_children[k].get();
								rynx_assert(child2 != nullptr, "node cannot be null");
								rynx_assert(child2 != child1, "nodes must differ");

								if ((child1->pos - child2->pos).lengthSquared() < sqr(child1->radius + child2->radius)) {
									collision_check_node_pair.emplace_back(child1, child2);
								}
							}
						}

						if (!collision_check_node_pair.empty()) {
							task.extend_task_execute_parallel([f, a, data = std::move(collision_check_node_pair)]() {
								for (auto entry : data) {
									collisions_internal(f, entry.first, entry.second);
								}
							});
						}
					}
				}
			});

			{
				std::vector<const node*> next_level;
				for (const node* node_ptr : current_level) {
					for (auto& child_ptr : node_ptr->m_children) {
						if (!child_ptr->m_children.empty()) {
							next_level.emplace_back(child_ptr.get());
						}
					}
				}
				current_level = std::move(next_level);
			}
			
			if (current_level.empty()) {
				return;
			}
		}
	}

	template<typename F> static void collisions_internal(F&& f, const node* rynx_restrict a) {
		rynx_assert(a != nullptr, "node cannot be null");

		if (a->m_children.empty()) {
			for (size_t i = 0; i < a->m_members.size(); ++i) {
				const auto& m1 = a->m_members[i];
				for (size_t k = i + 1; k < a->m_members.size(); ++k) {
					const auto& m2 = a->m_members[k];
					float distSqr = (m1.pos - m2.pos).lengthSquared();
					float radiusSqr = sqr(m1.radius + m2.radius);
					if (distSqr < radiusSqr) {
						f(m1.entityId, m2.entityId, (m1.pos - m2.pos).normalizeApprox(), std::sqrtf(radiusSqr) - std::sqrtf(distSqr));
					}
				}
			}
		}
		else {
			for (const auto& child : a->m_children) {
				collisions_internal(std::forward<F>(f), child.get());
			}

			for (size_t i = 0; i < a->m_children.size(); ++i) {
				const node* child1 = a->m_children[i].get();
				rynx_assert(child1 != nullptr, "node cannot be null");
				rynx_assert(child1 != a, "nodes must differ");

				for (size_t k = i + 1; k < a->m_children.size(); ++k) {
					const node* child2 = a->m_children[k].get();
					rynx_assert(child2 != nullptr, "node cannot be null");
					rynx_assert(child2 != child1, "nodes must differ");

					if ((child1->pos - child2->pos).lengthSquared() < sqr(child1->radius + child2->radius)) {
						collisions_internal(std::forward<F>(f), child1, child2);
					}
				}
			}
		}
	}

	template<typename F> static void collisions_internal(F&& f, const node* rynx_restrict a, const node* rynx_restrict b) {
		rynx_assert(a != nullptr, "node cannot be null");
		rynx_assert(b != nullptr, "node cannot be null");
		rynx_assert(a != b, "nodes must differ");

		if ((b->pos - a->pos).lengthSquared() > sqr(a->radius + b->radius)) {
			return;
		}

		// if b reaches bottom first, swap. this means less checks later.
		if (b->m_children.empty()) {
			const node* tmp = a;
			a = b;
			b = tmp;
		}

		if (a->m_children.empty()) {
			if (b->m_children.empty()) {
				if (a->radius < b->radius) {
					const node* tmp = a;
					a = b;
					b = tmp;
				}

				for (const auto& member1 : a->m_members) {
					if ((member1.pos - b->pos).lengthSquared() < sqr(member1.radius + b->radius)) {
						for (const auto& member2 : b->m_members) {
							float distSqr = (member1.pos - member2.pos).lengthSquared();
							float radiusSqr = sqr(member1.radius + member2.radius);
							if (distSqr < radiusSqr) {
								auto normal = (member1.pos - member2.pos).normalizeApprox();
								float penetration = std::sqrtf(radiusSqr) - std::sqrtf(distSqr);
								f(member1.entityId, member2.entityId, normal, penetration);
							}
						}
					}
				}
			}
			else {
				for (const auto& child : b->m_children) {
					collisions_internal(std::forward<F>(f), a, child.get());
				}
			}
		}
		else {
			if (a->radius > b->radius) {
				for (const auto& child : a->m_children) {
					collisions_internal(std::forward<F>(f), child.get(), b);
				}
			}
			else {
				for (const auto& child : b->m_children) {
					collisions_internal(std::forward<F>(f), a, child.get());
				}
			}
		}
	}
};
