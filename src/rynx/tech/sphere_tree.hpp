#pragma once

#include <rynx/tech/math/vector.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/math/geometry.hpp>
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
		node(vec3<float> pos, node* parent, int depth = 0) : pos(pos), parent(parent), depth(depth) {}

		vec3<float> pos;
		float radius = 0;
		int depth = 0;

		void killMePlease(node* child) {
			// TODO: nodes could have this index stored. is not hard.
			for (size_t i = 0; i < children.size(); ++i) {
				if (children[i].get() == child) {
					killMePlease(int(i));
					return;
				}
			}

			rynx_assert(false, "must find the kill");
		}

		void killMePlease(int index) {
			children[index] = std::move(children.back());
			children.pop_back();

			// if 1 child left, add child to parent. and kill self.
			if (children.size() == 1) {
				if (parent) {
					children[0]->parent = parent;
					parent->children.emplace_back(std::move(children[0]));
					parent->killMePlease(this);
				}
			}
		}

		uint64_t entity_migrates(index_t member_index, sphere_tree* container) {
			auto id_of_erased = members[member_index].entityId;
			members[member_index] = std::move(members.back());
			container->entryMap.find(members[member_index].entityId)->second.second = member_index;
			members.pop_back();

			if (members.empty() && parent != nullptr) {
				parent->killMePlease(this);
			}

			// TODO: check, if me and siblings are too many in number vs. the number of elements stored,
			// merge some of the siblings?
			return id_of_erased;
		}

		std::unique_ptr<node> node_migrates(node* child_ptr) {
			for (size_t i = 0; i < children.size(); ++i) {
				if (children[i].get() == child_ptr) {
					std::unique_ptr<node> child = std::move(children[i]);
					children[i] = std::move(children.back());
					children.pop_back();
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
			rynx_assert(children.empty(), "insert can only be done on leaf nodes");
			members.emplace_back(std::move(item));
			container->entryMap.insert_or_assign(members.back().entityId, std::pair<node*, index_t>(this, index_t(members.size() - 1)));

			if (members.size() >= MaxElementsInNode) {
				// if we have no parent (we are root node), add a couple of new child nodes under me.
				if (!parent) {
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
						children.emplace_back(std::make_unique<node>(members[i].pos, this, depth + 1));
					}
					for (size_t i = 0; i < members.size(); ++i) {
						children[i % children.size()]->insert(std::move(members[i]), container);
					}
					members.clear();

					for (auto& child : children) {
						child->update_single();
					}
				}
				else {
					// if also our parent node is full, and can't have new children.
					// add a new layer of nodes between my parent, and myself.
					if (parent->children.size() >= MaxNodesInNode) {
						// first take the existing children to safety.
						std::vector<std::unique_ptr<node>> children_of_parent = std::move(parent->children);

#if 1
						parent->children.clear();
						for (size_t i = 0; i < MaxNodesInNode; ++i) {
							parent->children.emplace_back(std::make_unique<node>(children_of_parent[i]->pos, parent, parent->depth + 1));
						}

						node* old_parent = parent;
						for (size_t i = 0; i < children_of_parent.size(); ++i) {
							auto& old_child = children_of_parent[i];
							auto& new_parent = old_parent->children[i % old_parent->children.size()];
							old_child->parent = new_parent.get();
							new_parent->children.emplace_back(std::move(old_child));
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
						for (auto& child : old_parent->children) {
							child->refresh_depths();
						}
					}

					// now there should be space in parent node's children list, add new siblings to self there.
					auto f1 = farPoint(members.back().pos, members);
					{
						parent->children.emplace_back(std::make_unique<node>(members[f1.first].pos, parent, depth));
						auto id = members[f1.first].entityId;
						parent->children.back()->members.emplace_back(std::move(members[f1.first]));

						auto& datap = container->entryMap.find(id)->second;
						datap.first = parent->children.back().get();
						datap.second = 0; // update mapping of the moved child!
					}

					if (f1.first != members.size() - 1) {
						auto id = members.back().entityId;
						members[f1.first] = std::move(members.back());
						container->entryMap.find(id)->second.second = index_t(f1.first); // also need to update mapping of new f1.first
					}

					members.pop_back();

					// these probably should not be called all the time. only after all move ops are done.
					parent->children.back()->update_single();
				}
			}
		}

		void refresh_depths() {
			for (auto& child : children) {
				child->depth = depth + 1;
				child->refresh_depths();
			}
		}

		// recalculate node position and radius
		void update_from_root_to_leaf() {
			update_single();
			if (parent) {
				parent->update_from_root_to_leaf();
			}
		}

		void update_from_leaf_to_root() {
			for (size_t i = 0; i < children.size(); ++i) {
				auto& child = children[i];
				if (child->children.empty() & child->members.empty()) {
					killMePlease(int(i));
					--i;
				}
				else {
					child->update_from_leaf_to_root();
				}
			}
			update_single();
		}

		void update_single() {
			std::pair<vec3<float>, float> posInfo;
			if (children.empty()) {
				posInfo = bounding_sphere(members);
			}
			else {
				posInfo = bounding_sphere(children);
			}

			pos = posInfo.first;
			radius = posInfo.second;
		}

		void update_parents_for_nodes() {
			if (parent) {
				auto new_parent = root()->find_nearest_parent(pos, depth, std::numeric_limits<float>::max());
				if (
					new_parent.first &&
					(new_parent.first != parent) &&
					new_parent.first->members.empty()
					) {
					std::unique_ptr<node> myself = parent->node_migrates(this);
					myself->parent = new_parent.first;
					new_parent.first->children.emplace_back(std::move(myself));
					new_parent.first->update_single();
				}
			}

			for (size_t i = 0; i < children.size(); ++i) {
				children[i]->update_parents_for_nodes();
			}
		}

		node* root() {
			if (!parent)
				return this;
			
			node* next = parent;
			while (true) {
				if (!next->parent)
					return next;
				next = next->parent;
			}
		}

		std::pair<node*, float> find_nearest_parent(vec3<float> point, int source_depth, float maxDistSqr) {
			std::pair<node*, float> ans(nullptr, maxDistSqr);
			for (auto&& child : children) {
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
							if (!child->children.empty()) {
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
			for (auto& child : children) {
				child->forEachNode(std::forward<F>(f), depth + 1);
			}
		}

		template<typename F>
		void inRange(vec3<float> point, float range, F&& f) {
			if ((pos - point).lengthSquared() < sqr(radius + range)) {
				if (children.empty()) {
					for (auto&& item : members) {
						if ((item.pos - point).lengthSquared() < range * range) {
							f(item.entityId, item.pos, item.radius);
						}
					}
				}
				else {
					for (auto&& child : children) {
						child->inRange(point, range, std::forward<F>(f));
					}
				}
			}
		}

		std::pair<node*, float> findNearestLeaf(vec3<float> point, float maxDistSqr) {
			if (children.empty()) {
				return { this, (point - pos).lengthSquared() };
			}

			std::pair<node*, float> ans(nullptr, maxDistSqr);
			for (auto&& child : children) {
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

		node* parent = nullptr;
		std::vector<std::unique_ptr<node>> children; // child nodes
		std::vector<entry> members;
	};

	rynx::unordered_map<uint64_t, std::pair<node*, index_t>> entryMap;
	node root;
	size_t update_next_index = 0;

public:
	sphere_tree() : root({0, 0, 0}, nullptr) {}

	void updateEntity(vec3<float> pos, float radius, uint64_t entityId) {
		auto it = entryMap.find(entityId);
		if (it != entryMap.end()) {
			auto& data = it->second.first->members[it->second.second];
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
			rynx_profile(Game, "FindBuckets");
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

				auto item = it->second.first->members[it->second.second];
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
			root.update_parents_for_nodes();
		}

		{
			rynx_profile(Game, "UpdateNodePositions");
			root.update_from_leaf_to_root();
		}
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
	template<typename F> static void collisions_internal_parallel(F&& f, rynx::scheduler::task& task, node* rynx_restrict a) {
		rynx_assert(a != nullptr, "node cannot be null");

		if (a->children.empty()) {
			for (size_t i = 0; i < a->members.size(); ++i) {
				auto& m1 = a->members[i];
				for (size_t k = i + 1; k < a->members.size(); ++k) {
					auto& m2 = a->members[k];
					float distSqr = (m1.pos - m2.pos).lengthSquared();
					float radiusSqr = sqr(m1.radius + m2.radius);
					if (distSqr < radiusSqr) {
						f(m1.entityId, m2.entityId, (m1.pos - m2.pos).normalizeApprox(), std::sqrtf(radiusSqr) - std::sqrtf(distSqr));
					}
				}
			}
		}
		else {
			task.extend_task_shared_resources([f, a](rynx::scheduler::task& task) {
				for (auto& child : a->children) {
					collisions_internal_parallel(f, task, child.get());
				}
			});

			task.extend_task_copy_resources([f, a](rynx::scheduler::task& task) {

				// for small nodes, don't bother creating tasks.
				if (a->children.size() < 30) {
					for (size_t i = 0; i < a->children.size(); ++i) {
						node* child1 = a->children[i].get();
						rynx_assert(child1 != nullptr, "node cannot be null");
						rynx_assert(child1 != a, "nodes must differ");

						for (size_t k = i + 1; k < a->children.size(); ++k) {
							node* child2 = a->children[k].get();
							rynx_assert(child2 != nullptr, "node cannot be null");
							rynx_assert(child2 != child1, "nodes must differ");

							if ((child1->pos - child2->pos).lengthSquared() < sqr(child1->radius + child2->radius)) {
								collisions_internal(f, child1, child2);
							}
						}
					}
					return;
				}

				auto handle_two_ranges = [f, a](size_t start1, size_t end1, size_t start2, size_t end2) {
					for (size_t i = start1; i < end1; ++i) {
						node* child1 = a->children[i].get();
						rynx_assert(child1 != nullptr, "node cannot be null");
						rynx_assert(child1 != a, "nodes must differ");

						for (size_t k = start2; k < end2; ++k) {
							node* child2 = a->children[k].get();
							rynx_assert(child2 != nullptr, "node cannot be null");
							rynx_assert(child2 != child1, "nodes must differ");

							if ((child1->pos - child2->pos).lengthSquared() < sqr(child1->radius + child2->radius)) {
								collisions_internal(f, child1, child2);
							}
						}
					}
				};
				
				auto handle_one_range = [f, a](size_t start, size_t end) {
					for (size_t i = start; i < end; ++i) {
						node* child1 = a->children[i].get();
						rynx_assert(child1 != nullptr, "node cannot be null");
						rynx_assert(child1 != a, "nodes must differ");

						for (size_t k = i+1; k < end; ++k) {
							node* child2 = a->children[k].get();
							rynx_assert(child2 != nullptr, "node cannot be null");
							rynx_assert(child2 != child1, "nodes must differ");

							if ((child1->pos - child2->pos).lengthSquared() < sqr(child1->radius + child2->radius)) {
								collisions_internal(f, child1, child2);
							}
						}
					}
				};

				int total_size = int(a->children.size());
				static constexpr const int num_segments = 4;
				int one_segment_size = total_size / num_segments;
				
				auto two_ranges = [=](int range1, int range2) {
					rynx_assert(total_size == a->children.size(), "??");
					rynx_assert(range1 < range2, "wrongful call.");
					if (range2 == 3) {
						handle_two_ranges(one_segment_size * range1, one_segment_size * (range1 + 1), one_segment_size * range2, total_size);
					}
					else {
						handle_two_ranges(one_segment_size * range1, one_segment_size * (range1 + 1), one_segment_size * range2, one_segment_size * (range2 + 1));
					}
				};
				auto one_range = [=](int a) { handle_one_range(one_segment_size * a, one_segment_size * (a + 1)); };

				// intra-segment checks.
				{
					task.extend_task_copy_resources([=](rynx::scheduler::task& task) {
						task.extend_task_shared_resources([=]() { rynx_assert(total_size == a->children.size(), "??"); one_range(0); });
						task.extend_task_shared_resources([=]() { rynx_assert(total_size == a->children.size(), "??"); one_range(1); });
						task.extend_task_shared_resources([=]() { rynx_assert(total_size == a->children.size(), "??"); one_range(2); });
						task.extend_task_shared_resources([=]() {
							rynx_assert(total_size == a->children.size(), "??");
							handle_one_range(one_segment_size * (num_segments - 1), total_size); // in case size mod num_segments is not zero.
						});
					});
				}

				// segment pair checks.
				{
					task.extend_task_copy_resources([=](rynx::scheduler::task& task) {
						task.extend_task_shared_resources([=]() {
							two_ranges(0, 1);
						});
						task.extend_task_shared_resources([=]() {
							two_ranges(2, 3);
						});
					});

					task.extend_task_copy_resources([=](rynx::scheduler::task& task) {
						task.extend_task_shared_resources([=]() {
							two_ranges(0, 2);
						});
						task.extend_task_shared_resources([=]() {
							two_ranges(1, 3);
						});
					});

					task.extend_task_copy_resources([f, a, two_ranges, handle_two_ranges](rynx::scheduler::task& task) {
						task.extend_task_shared_resources([f, a, two_ranges, handle_two_ranges]() {
							two_ranges(0, 3);
						});
						task.extend_task_shared_resources([f, a, two_ranges, handle_two_ranges]() {
							two_ranges(1, 2);
						});
					});
				}
			});
		}
	}

	template<typename F> static void collisions_internal(F&& f, node* rynx_restrict a) {
		rynx_assert(a != nullptr, "node cannot be null");

		if (a->children.empty()) {
			for (size_t i = 0; i < a->members.size(); ++i) {
				auto& m1 = a->members[i];
				for (size_t k = i + 1; k < a->members.size(); ++k) {
					auto& m2 = a->members[k];
					float distSqr = (m1.pos - m2.pos).lengthSquared();
					float radiusSqr = sqr(m1.radius + m2.radius);
					if (distSqr < radiusSqr) {
						f(m1.entityId, m2.entityId, (m1.pos - m2.pos).normalizeApprox(), std::sqrtf(radiusSqr) - std::sqrtf(distSqr));
					}
				}
			}
		}
		else {
			for (auto& child : a->children) {
				collisions_internal(std::forward<F>(f), child.get());
			}

			for (size_t i = 0; i < a->children.size(); ++i) {
				node* child1 = a->children[i].get();
				rynx_assert(child1 != nullptr, "node cannot be null");
				rynx_assert(child1 != a, "nodes must differ");

				for (size_t k = i + 1; k < a->children.size(); ++k) {
					node* child2 = a->children[k].get();
					rynx_assert(child2 != nullptr, "node cannot be null");
					rynx_assert(child2 != child1, "nodes must differ");

					if ((child1->pos - child2->pos).lengthSquared() < sqr(child1->radius + child2->radius)) {
						collisions_internal(std::forward<F>(f), child1, child2);
					}
				}
			}
		}
	}

	template<typename F> static void collisions_internal(F&& f, node* rynx_restrict a, node* rynx_restrict b) {
		rynx_assert(a != nullptr, "node cannot be null");
		rynx_assert(b != nullptr, "node cannot be null");
		rynx_assert(a != b, "nodes must differ");

		if ((b->pos - a->pos).lengthSquared() > sqr(a->radius + b->radius)) {
			return;
		}

		// if b reaches bottom first, swap. this means less checks later.
		if (b->children.empty()) {
			node* tmp = a;
			a = b;
			b = tmp;
		}

		if (a->children.empty()) {
			if (b->children.empty()) {
				if (a->radius < b->radius) {
					node* tmp = a;
					a = b;
					b = tmp;
				}

				for (auto& member1 : a->members) {
					if ((member1.pos - b->pos).lengthSquared() < sqr(member1.radius + b->radius)) {
						for (auto& member2 : b->members) {
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
				for (auto& child : b->children) {
					collisions_internal(std::forward<F>(f), a, child.get());
				}
			}
		}
		else {
			if (a->radius > b->radius) {
				for (auto& child : a->children) {
					collisions_internal(std::forward<F>(f), child.get(), b);
				}
			}
			else {
				for (auto& child : b->children) {
					collisions_internal(std::forward<F>(f), a, child.get());
				}
			}
		}
	}
};