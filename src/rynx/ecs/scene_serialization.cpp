
#include <rynx/std/serialization_declares.hpp>
#include <rynx/ecs/scene_serialization.hpp>
#include <rynx/ecs/raw_serialization.hpp>
#include <rynx/ecs/ecs.hpp>
#include <rynx/math/geometry/math.hpp>

namespace {
	struct subscene_edits {
		struct component_erase {
			int32_t path_index;
			rynx::string component_name;
		};

		struct component_edit {
			int32_t path_index;
			std::vector<char> serialized_component;
			rynx::string component_name;
		};

		std::vector<int32_t> deleted_entities; // entity path indices for deleted entities
		std::vector<component_erase> erased_components;
		std::vector<component_edit> added_components;
		std::vector<component_edit> edited_components;
	};
}

namespace rynx::serialization {
	template<> struct Serialize<subscene_edits> {
		template<typename IOStream>
		void serialize(const subscene_edits& t, IOStream& writer) {
			rynx::serialize(t.deleted_entities, writer);
			rynx::serialize(t.added_components, writer);
			rynx::serialize(t.erased_components, writer);
			rynx::serialize(t.edited_components, writer);
		}

		template<typename IOStream>
		void deserialize(subscene_edits& t, IOStream& reader) {
			rynx::deserialize(t.deleted_entities, reader);
			rynx::deserialize(t.added_components, reader);
			rynx::deserialize(t.erased_components, reader);
			rynx::deserialize(t.edited_components, reader);
		}
	};

	template<> struct Serialize<subscene_edits::component_erase> {
		template<typename IOStream>
		void serialize(const subscene_edits::component_erase& t, IOStream& writer) {
			rynx::serialize(t.component_name, writer);
			rynx::serialize(t.path_index, writer);
		}

		template<typename IOStream>
		void deserialize(subscene_edits::component_erase& t, IOStream& reader) {
			rynx::deserialize(t.component_name, reader);
			rynx::deserialize(t.path_index, reader);
		}
	};

	template<> struct Serialize<subscene_edits::component_edit> {
		template<typename IOStream>
		void serialize(const subscene_edits::component_edit& t, IOStream& writer) {
			rynx::serialize(t.component_name, writer);
			rynx::serialize(t.path_index, writer);
			rynx::serialize(t.serialized_component, writer);
		}

		template<typename IOStream>
		void deserialize(subscene_edits::component_edit& t, IOStream& reader) {
			rynx::deserialize(t.component_name, reader);
			rynx::deserialize(t.path_index, reader);
			rynx::deserialize(t.serialized_component, reader);
		}
	};
}

// TODO: Move elsewhere
// finds the parent scene link of a given entity.
// if no scene parent is found, returns invalid id.
rynx::id rynx::ecs_detail::scene_serializer::scene_parent_of(rynx::id entity) {
	auto entity_obj = host->operator[](entity);
	if (entity_obj.has<rynx::components::scene::parent>()) {
		return entity_obj.get<rynx::components::scene::parent>().entity;
	}
	return rynx::id();
}

std::vector<rynx::components::scene::persistent_id> rynx::ecs_detail::scene_serializer::persistent_id_path_create(rynx::id to) {
	id to_scene = scene_parent_of(to);

	// if entity is in root scene, just return the target entity persistent id as the path.
	if (!to_scene)
		return { host->operator[](to).get<rynx::components::scene::persistent_id>() };

	std::vector<rynx::components::scene::persistent_id> path;
	path.emplace_back(host->operator[](to).get<rynx::components::scene::persistent_id>());

	id reverse_path_iteration = to_scene;
	while (true) {
		rynx::components::scene::persistent_id persistent_id_of_current_node = host->operator[](reverse_path_iteration).get<rynx::components::scene::persistent_id>();
		path.emplace_back(persistent_id_of_current_node);
		reverse_path_iteration = scene_parent_of(reverse_path_iteration);
		if (!reverse_path_iteration) {
			std::reverse(path.begin(), path.end());
			rynx_assert(persistent_id_path_find(path) == to, "created path does not point to the desired target");
			return path;
		}
		rynx_assert(reverse_path_iteration.operator bool(), "path not found. target is in wrong direction in the tree?");
	}

	rynx_assert(false, "unreachable");
}

std::vector<rynx::components::scene::persistent_id> rynx::ecs_detail::scene_serializer::persistent_id_path_create(rynx::id from, rynx::id to) {
	id from_scene = scene_parent_of(from);
	id to_scene = scene_parent_of(to);

	// if entities are in the same scene, just return the target entity persistent id as the path.
	if (from_scene == to_scene)
		return { host->operator[](to).get<rynx::components::scene::persistent_id>() };

	std::vector<rynx::components::scene::persistent_id> path;
	path.emplace_back(host->operator[](to).get<rynx::components::scene::persistent_id>());

	id reverse_path_iteration = to_scene;
	while (true) {
		path.emplace_back(host->operator[](reverse_path_iteration).get<rynx::components::scene::persistent_id>());
		reverse_path_iteration = scene_parent_of(reverse_path_iteration);
		if (reverse_path_iteration == from_scene) {
			std::reverse(path.begin(), path.end());
			return path;
		}
		rynx_assert(reverse_path_iteration.operator bool(), "path not found. target is in wrong direction in the tree?");
	}

	rynx_assert(false, "unreachable");
}

rynx::id rynx::ecs_detail::scene_serializer::persistent_id_path_find(rynx::entity_range_t range, rynx::id from, const std::vector<rynx::components::scene::persistent_id>& path) {
	id from_scene = scene_parent_of(from);

	int32_t current_path_index = 0;
	auto find_next_child = [this, &path](rynx::id current, int32_t current_path_index) {
		const auto& children = host->operator[](current).get<rynx::components::scene::children>();
		for (auto id : children.entities) {
			if (host->operator[](id).get<rynx::components::scene::persistent_id>().value == path[current_path_index].value) {
				return id;
			}
		}
		rynx_assert(false, "unreachable");
		return id();
	};

	rynx::id current = from;
	if (!from_scene) {
		current = [this, range, path]() {
			auto ids = host->query().notIn<rynx::components::scene::parent>().ids();
			for (auto id : ids) {
				// Only consider entities from currently deserialized scene.
				if (!range.contains(id))
					continue;

				if (host->operator[](id).get<rynx::components::scene::persistent_id>().value == path[0].value) {
					return id;
				}
			}
			rynx_assert(false, "unreachable");
			return id();
		}();
		++current_path_index;
	}

	while (current_path_index < path.size()) {
		current = find_next_child(current, current_path_index);
		++current_path_index;
	}
	return current;
}

rynx::id rynx::ecs_detail::scene_serializer::persistent_id_path_find(const std::vector<rynx::components::scene::persistent_id>& path) {
	rynx::id from_scene;

	int32_t current_path_index = 0;
	auto find_next_child = [this, &path](rynx::id current, int32_t current_path_index) {
		const auto& children = host->operator[](current).get<rynx::components::scene::children>();
		for (auto id : children.entities) {
			if (host->operator[](id).get<rynx::components::scene::persistent_id>().value == path[current_path_index].value) {
				return id;
			}
		}
		logmsg("WARNING: requested id not found, edit of subscene can't be applied");
		return id();
	};

	auto ids = host->query().notIn<rynx::components::scene::parent>().in<rynx::components::scene::persistent_id>().ids();
	auto current = [this, &path, &ids]() {
		for (auto id : ids) {
			if (host->operator[](id).get<rynx::components::scene::persistent_id>().value == path[0].value) {
				return id;
			}
		}
		rynx_assert(false, "unreachable");
		return id();
	}();
	++current_path_index;

	while (current_path_index < path.size()) {
		current = find_next_child(current, current_path_index);
		++current_path_index;
	}
	return current;
}

// serializes the top-most scene to memory.
void rynx::ecs_detail::scene_serializer::serialize_scene(
	rynx::reflection::reflections& reflections,
	rynx::filesystem::vfs& vfs,
	rynx::scenes& scenes,
	rynx::serialization::vector_writer& out) {

	// ensure all entities have a scene-persistent id
		{
			// find current largest persistent id in root scene.
			int32_t max_persistent_id_value = 0;
			host->query()
				.in<rynx::components::scene::persistent_id>()
				.notIn<rynx::components::scene::parent>()
				.for_each([&max_persistent_id_value](rynx::components::scene::persistent_id id) {
				max_persistent_id_value = id.value > max_persistent_id_value ? id.value : max_persistent_id_value;
					});

			// NOTE: all entities in any subscene are guaranteed to have this id already (would not be serialized otherwise)
			//       therefore, only entities in current root scene can possibly match this query.
			{
				auto ids = host->query().notIn<rynx::components::scene::persistent_id>().ids();
				for (auto id : ids)
					host->attachToEntity(id, rynx::components::scene::persistent_id{ ++max_persistent_id_value });
			}
		}

		std::vector<std::vector<rynx::components::scene::persistent_id>> path_collection;
		auto add_path_to_collection_and_get_index = [&path_collection](std::vector<rynx::components::scene::persistent_id>&& path) {
			size_t collection_size = path_collection.size();
			for (size_t i = 0; i < collection_size; ++i) {
				if (path_collection[i] == path) {
					return int32_t(i);
				}
			}
			path_collection.emplace_back(std::move(path));
			return int32_t(collection_size);
		};

		subscene_edits edits;

		// handle edits made to subscenes (their data is not serialized here so the edits must be stored separately)
		{
			// create a new empty ecs instance to describe how the sub scenes are expected to look like (for constructing a diff for serialization)
			rynx::ecs expected;

			// find all scene links without a parent in current scene (direct descendants of current scene)
			auto top_most_child_scenes = host->query()
				.notIn<rynx::components::scene::parent>()
				.gather<rynx::components::transform::position, rynx::components::scene::link, rynx::components::scene::persistent_id>();

			// copy the scene links into empty scene, and deserialize scenes there
			for (auto [pos, link, persistent_id] : top_most_child_scenes)
				expected.create(pos, link, persistent_id);
			
			rynx::ecs_detail::scene_serializer{&expected}.load_subscenes(reflections, vfs, scenes);

			// for every entity in current scene with a parent component:
			auto subscene_entities = host->query().in<rynx::components::scene::parent>().ids();
			for (auto id : subscene_entities) {

				// find the matching entity in the new ecs instance via persistent id paths
				auto matching_entity = rynx::ecs_detail::scene_serializer{ &expected }.persistent_id_path_find(persistent_id_path_create(id));
				if (!matching_entity) {
					// if matching component does not exist, serialize and add to "added components" struct
					// NOTE: what the fuck is this supposed to mean? Entities are not added to subscenes.
					//       if an entity is added, it is added to the root scene.
					continue;
				}

				// if matching entity is found, for each component check that they are the same
				auto [actual_category_ptr, actual_category_index] = host->m_idCategoryMap.find(id)->second;
				auto [expected_category_ptr, expected_category_index] = expected.m_idCategoryMap.find(matching_entity)->second;

				const auto& actual_types = actual_category_ptr->types();
				const auto& expected_types = expected_category_ptr->types();

				// removed components (in expected, not in actual)
				for (uint64_t type_id_value : expected_types) {
					if (!actual_types.test(type_id_value)) {
						const auto* type_reflection = reflections.find(type_id_value);
						if (!type_reflection) {
							continue;
						}
						int32_t path_collection_index = add_path_to_collection_and_get_index(persistent_id_path_create(id));
						edits.erased_components.emplace_back(
							subscene_edits::component_erase{
								path_collection_index,
								type_reflection->m_type_name
							}
						);
					}
				}

				// added components (not in expected, in actual)
				for (uint64_t type_id_value : actual_types) {
					if (!expected_types.test(type_id_value)) {
						const auto* type_reflection = reflections.find(type_id_value);
						if (!type_reflection) {
							continue;
						}
						if (!type_reflection->m_serialization_allowed) {
							continue;
						}

						logmsg("subscene entity component ADDED: '%s' for entity %d", type_reflection->m_type_name.c_str(), int32_t(id.value));

						if (type_reflection->m_fields.empty()) {
							int32_t path_collection_index = add_path_to_collection_and_get_index(persistent_id_path_create(id));
							edits.added_components.emplace_back(
								subscene_edits::component_edit{
									path_collection_index,
									{},
									type_reflection->m_type_name
								}
							);
						}
						else {
							auto& itable_actual = actual_category_ptr->table(rynx::type_id_t(type_id_value));
							int32_t path_collection_index = add_path_to_collection_and_get_index(persistent_id_path_create(id));

							// if edited component value has an id link, create a path to the new target
							// save target to paths, replace value with index, serialize as is.
							std::vector<rynx::id> original_id_link_values;
							itable_actual.for_each_id_field_for_single_index(actual_category_index, [this, &path_collection, &add_path_to_collection_and_get_index, &original_id_link_values](rynx::id& entity_link) mutable {
								original_id_link_values.emplace_back(entity_link);
								entity_link.value = add_path_to_collection_and_get_index(persistent_id_path_create(entity_link));
								rynx_assert(persistent_id_path_find(path_collection[entity_link.value]) == original_id_link_values.back(), "saved path does not point back");
							});

							rynx::serialization::vector_writer memory_writer;
							itable_actual.serialize_index(memory_writer, actual_category_index);
							edits.added_components.emplace_back(
								subscene_edits::component_edit{
									path_collection_index,
									std::move(memory_writer.data()),
									itable_actual.type_name()
								}
							);

							auto original_values_iterator = original_id_link_values.begin();
							itable_actual.for_each_id_field_for_single_index(actual_category_index, [&original_values_iterator](rynx::id& entity_link) mutable {
								entity_link.value = *original_values_iterator;
								++original_values_iterator;
								});
						}
					}
				}

				// edited components
				for (uint64_t type_id_value : expected_types & actual_types) {
					const auto* type_reflection = reflections.find(type_id_value);
					
					// virtual type - no table
					if (!type_reflection)
						continue;

					// tag type - no table
					if (type_reflection->m_fields.empty())
						continue;
					
					logmsg("checking edit state of component '%s'", type_reflection->m_type_name.c_str());
					auto& itable_actual = actual_category_ptr->table(rynx::type_id_t(type_id_value));
					const auto& itable_expected = expected_category_ptr->table(rynx::type_id_t(type_id_value));
					bool components_are_equal = itable_actual.equals(actual_category_index, itable_expected.get(expected_category_index));

					if (!components_are_equal) {
						logmsg("subscene component EDITED: '%s' for entity %d", reflections.find(type_id_value)->m_type_name.c_str(), int32_t(id.value));
						int32_t path_collection_index = add_path_to_collection_and_get_index(persistent_id_path_create(id));
						logmsg("entity %lld, saving with path index %d", id.value, path_collection_index);
						logmsg("--reading from index %d, expecting to get entity id %lld", path_collection_index, persistent_id_path_find(path_collection[path_collection_index]).value);

						// if edited component value has an id link, create a path to the new target
						// save target to paths, replace value with index, serialize as is.
						std::vector<rynx::id> original_id_link_values;
						itable_actual.for_each_id_field_for_single_index(actual_category_index, [this, id, &path_collection, &add_path_to_collection_and_get_index, &original_id_link_values](rynx::id& entity_link) mutable {
							original_id_link_values.emplace_back(entity_link);
							entity_link.value = add_path_to_collection_and_get_index(persistent_id_path_create(entity_link));
							rynx_assert(persistent_id_path_find(path_collection[entity_link.value]) == original_id_link_values.back(), "saved path does not point back");
							logmsg("--id field edit serializer, saving for entity %lld, %lld->%lld", id.value, original_id_link_values.back().value, entity_link.value);
						});

						rynx::serialization::vector_writer memory_writer;
						itable_actual.serialize_index(memory_writer, actual_category_index);
						edits.edited_components.emplace_back(
							subscene_edits::component_edit{
								path_collection_index,
								std::move(memory_writer.data()),
								itable_actual.type_name()
							}
						);

						auto original_values_iterator = original_id_link_values.begin();
						itable_actual.for_each_id_field_for_single_index(actual_category_index, [&original_values_iterator](rynx::id& entity_link) mutable {
							entity_link.value = *original_values_iterator;
							++original_values_iterator;
						});
					}
				}
			}

			// removed entities
			auto expected_entities = expected.query().in<rynx::components::scene::parent>().ids();
			for (auto id : expected_entities) {

				// find the matching entity in the new ecs instance via persistent id paths
				auto entity_path = rynx::ecs_detail::scene_serializer{ &expected }.persistent_id_path_create(id);
				auto matching_entity = persistent_id_path_find(entity_path);
				if (!matching_entity) {
					// entity deleted
					edits.deleted_entities.emplace_back(int32_t(path_collection.size()));
					path_collection.emplace_back(std::move(entity_path));
				}
			}
		}

		// make a copy of ecs
		rynx::ecs copy = host->clone();

		{
			auto ids = copy.query()
				.notIn<rynx::components::scene::parent>() // only consider top level entities
				.ids();

			// construct mapping from id references to global id chains (for the top-most scene?).
			// replace id references with global id chain (for the top-most scene)
			for (auto id : ids) {
				auto [category_ptr, entity_index] = copy.m_idCategoryMap[id.value];
				category_ptr->forEachTable([&path_collection, &copy, id, entity_index](rynx::ecs_internal::itable* table_ptr) {
					table_ptr->for_each_id_field_for_single_index(entity_index, [&path_collection, &copy, id](rynx::id& target) {
						auto persistent_id_path = rynx::ecs_detail::scene_serializer{ &copy }.persistent_id_path_create(id, target);
						auto it = [&path_collection, &persistent_id_path]() {
							for (auto it = path_collection.begin(); it != path_collection.end(); ++it) {
								if (*it == persistent_id_path)
									return it;
							}
							return path_collection.end();
						}();

						if (it == path_collection.end()) {
							target = path_collection.size();
							path_collection.emplace_back(std::move(persistent_id_path));
						}
						else {
							int32_t index = int32_t(it - path_collection.begin());
							target = index;
						}
					});
				});
			}
		}

		// remove all sub-scene content
		auto res = copy.query().gather<rynx::components::scene::children>();
		for (auto [children_component] : res) {
			for (auto id : children_component.entities) {
				copy.erase(id);
			}
		}

		// serialize global id chains vector
		rynx::serialize(path_collection, out);
		
		// serialize subscene edits
		rynx::serialize(edits, out);
		
		// serialize ecs data as-is.
		rynx::ecs_detail::raw_serializer{ &copy }.serialize(reflections, out);
}

rynx::serialization::vector_writer rynx::ecs_detail::scene_serializer::serialize_scene(rynx::reflection::reflections& reflections,
	rynx::filesystem::vfs& vfs,
	rynx::scenes& scenes) {
	rynx::serialization::vector_writer out;
	serialize_scene(reflections, vfs, scenes, out);
	return out;
}

// deserializes a scene and any sub-scenes referenced in it.
// returns an entity range of ids in scene (excluding subscenes)
// and an entity range of all ids deserialized (including subscenes)
std::tuple<rynx::entity_range_t, rynx::entity_range_t> rynx::ecs_detail::scene_serializer::deserialize_scene(
	rynx::reflection::reflections& reflections,
	rynx::filesystem::vfs& vfs,
	rynx::scenes& scenes,
	rynx::components::transform::position scene_pos,
	rynx::serialization::vector_reader& in) {

	// deserialize global id chains vector
	std::vector<std::vector<rynx::components::scene::persistent_id>> path_collection = rynx::deserialize<std::vector<std::vector<rynx::components::scene::persistent_id>>>(in);

	// deserialize component edits
	subscene_edits edits = rynx::deserialize<subscene_edits>(in);

	// deserialize ecs as-is
	auto entity_id_range = rynx::ecs_detail::raw_serializer{ host }.deserialize(reflections, in);
	entity_range_t all_entities_id_range = entity_id_range;

	// transform all deserialized entities by scene root transform.
	{
		float sin_v = rynx::math::sin(scene_pos.angle);
		float cos_v = rynx::math::cos(scene_pos.angle);
		for (auto id : entity_id_range) {
			auto entity = (*host)[id];
			auto& entity_pos = entity.get<rynx::components::transform::position>();
			const auto local_pos = entity_pos;

			auto rotated_local_pos = rynx::math::rotatedXY(local_pos.value, sin_v, cos_v);
			entity_pos.value = rotated_local_pos + scene_pos.value;
			entity_pos.angle = scene_pos.angle + local_pos.angle;

			entity.add(rynx::components::scene::local_position{ local_pos });
		}
	}

	// for any sub-scene link deserialized, deserialize the content of the subscene (recursive call)
	{
		std::vector<std::tuple<rynx::id, rynx::components::scene::link, rynx::components::transform::position>> sub_scenes;
		host->query()
			.notIn<rynx::components::scene::children>()
			.for_each([entity_id_range, &sub_scenes](rynx::id id, rynx::components::scene::link scene_link, rynx::components::transform::position pos) {
			if (entity_id_range.contains(id)) {
				sub_scenes.emplace_back(id, scene_link, pos);
			}
				});

		for (auto [root_id, link, position] : sub_scenes) {
			auto sub_scene_blob = scenes.get(vfs, link.id);
			rynx::serialization::vector_reader sub_scene_in(std::move(sub_scene_blob));
			auto [subscene_ids, subscene_all_ids] = deserialize_scene(reflections, vfs, scenes, position, sub_scene_in);

			// update id range end.
			all_entities_id_range.m_end = subscene_all_ids.m_end;

			// add the children range to scene root
			{
				rynx::components::scene::children root_children;
				root_children.entities = subscene_ids;
				(*host)[root_id].add(root_children);
			}

			// add parent link to children.
			for (auto id : subscene_ids) {
				(*host)[id].add(rynx::components::scene::parent{ root_id });
			}
		}
	}

	// for each id link in current deserialized scene,
	//   look at corresponding chain in chains,
	//     track the chain down the scenes and find the entity pointed to by the chain
	for (auto id : entity_id_range) {
		auto [category_ptr, category_index] = host->m_idCategoryMap.find(id)->second;
		category_ptr->forEachTable([this, entity_id_range, &path_collection, id, category_index](rynx::ecs_internal::itable* table_ptr) {
			table_ptr->for_each_id_field_for_single_index(category_index, [this, entity_id_range, &path_collection, id](rynx::id& target) {
				target = persistent_id_path_find(entity_id_range, id, path_collection[target.value]);
			});
		});
	}

	// remove entities that were deleted in edits
	for (auto& deleted_entity_path_index : edits.deleted_entities) {
		auto entity_id = persistent_id_path_find(path_collection[deleted_entity_path_index]);
		logmsg("apply subscene entity erase for '%lld'", entity_id.value);
		if(entity_id)
			host->erase(entity_id);
	}

	// remove components that were erased in edits
	for (auto& erased_component : edits.erased_components) {
		auto entity_id = persistent_id_path_find(path_collection[erased_component.path_index]);
		if (entity_id) {
			const auto* type_reflection = reflections.find(erased_component.component_name);
			if (!type_reflection) {
				logmsg("subscene edit: 'component erase' failed; no type reflection available for '%s'", erased_component.component_name.c_str());
				continue;
			}

			logmsg("apply subscene component erase for '%s'", erased_component.component_name.c_str());
			host->removeFromEntity(entity_id, rynx::type_id_t(type_reflection->m_type_index_value));
		}
	}

	// add components that were introduced in edits
	for (auto& added_component : edits.added_components) {
		auto entity_id = persistent_id_path_find(path_collection[added_component.path_index]);
		if (entity_id) {
			const auto* type_reflection = reflections.find(added_component.component_name);
			if (!type_reflection) {
				logmsg("subscene edit: 'component add' failed; no type reflection available for '%s'", added_component.component_name.c_str());
				continue;
			}

			logmsg("apply subscene component add for '%s'", added_component.component_name.c_str());
			host->attachToEntity_typeErased(
				entity_id,
				type_reflection->m_type_index_value,
				type_reflection->m_is_type_segregated,
				type_reflection->m_create_table_func,
				type_reflection->m_create_map_func,
				type_reflection->m_deserialize_instance_func(added_component.serialized_component)
			);

			if (!type_reflection->m_fields.empty()) {
				auto [category_ptr, category_index] = host->m_idCategoryMap.find(entity_id)->second;
				auto& itable = category_ptr->table(type_reflection->m_type_index_value);
				itable.for_each_id_field_for_single_index(category_index, [this, &path_collection](rynx::id& target) {
					target = persistent_id_path_find(path_collection[target.value]);
				});
			}
		}
	}

	// modify components that were edited in edits.
	for (auto& edited_component : edits.edited_components) {
		auto entity_id = persistent_id_path_find(path_collection[edited_component.path_index]);
		if (entity_id) {
			const auto* type_reflection = reflections.find(edited_component.component_name);
			if (!type_reflection) {
				logmsg("subscene edit: 'component edit' failed; no type reflection available for '%s'", edited_component.component_name.c_str());
				continue;
			}
			logmsg("apply subscene edit for '%s'", edited_component.component_name.c_str());
			auto [category_ptr, category_index] = host->m_idCategoryMap.find(entity_id)->second;
			auto& table = category_ptr->table(type_reflection->m_type_index_value);
			rynx::serialization::vector_reader reader(edited_component.serialized_component);
			table.deserialize_index(reader, category_index);

			// if there were serialized id links, remap those back to actual entity values.
			auto& itable = category_ptr->table(type_reflection->m_type_index_value);
			itable.for_each_id_field_for_single_index(category_index, [this, entity_id_range, &path_collection](rynx::id& target) {
				logmsg("--id member remapping from path index %lld -> entity id %lld", target.value, persistent_id_path_find(path_collection[target.value]).value);
				target = persistent_id_path_find(path_collection[target.value]);
			});
		}
	}

	return { entity_id_range, all_entities_id_range };
}

rynx::entity_range_t rynx::ecs_detail::scene_serializer::load_subscenes(rynx::reflection::reflections& reflections, rynx::filesystem::vfs& vfs, rynx::scenes& scenes) {
	std::vector<std::tuple<rynx::id, rynx::components::scene::link, rynx::components::transform::position>> sub_scenes;
	host->query()
		.notIn<rynx::components::scene::children>()
		.for_each([&sub_scenes](rynx::id id, rynx::components::scene::link scene_link, rynx::components::transform::position pos) {
		sub_scenes.emplace_back(id, scene_link, pos);
	});

	rynx::entity_range_t all_entities{ host->m_entities.peek_next_id(), host->m_entities.peek_next_id() };

	for (auto [root_id, link, scene_pos] : sub_scenes) {
		auto sub_scene_blob = scenes.get(vfs, link.id);
		rynx::serialization::vector_reader sub_scene_in(std::move(sub_scene_blob));
		auto [subscene_ids, subscene_ids_recursive] = deserialize_scene(reflections, vfs, scenes, scene_pos, sub_scene_in);

		// update id range end.
		all_entities.m_end = subscene_ids_recursive.m_end;

		// add the children range to scene root
		{
			// TODO: child entities may die during simulation, this is not taken into account.
			// is this an issue?
			rynx::components::scene::children root_children;
			root_children.entities = subscene_ids;
			(*host)[root_id].add(root_children);
		}

		// add parent link to children.
		for (auto id : subscene_ids) {
			(*host)[id].add(rynx::components::scene::parent{ root_id });
		}
	}

	return all_entities;
}
