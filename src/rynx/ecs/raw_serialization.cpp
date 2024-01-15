
#include <rynx/ecs/raw_serialization.hpp>
#include <rynx/ecs/ecs.hpp>

void rynx::ecs_detail::raw_serializer::serialize(rynx::reflection::reflections& reflections, rynx::serialization::vector_writer& out) {
	// first construct mapping from ecs ids to serialized ids.
	rynx::unordered_map<uint64_t, uint64_t> idToSerializedId;
	rynx::unordered_map<uint64_t, uint64_t> serializedIdToId;
	uint64_t next = 1;
	for (auto&& id : host->m_idCategoryMap) {
		idToSerializedId[id.first] = next;
		serializedIdToId[next] = id.first;
		++next;
	}

	if constexpr (false) {
		// replace id fields with serialized id values.
		for (auto&& category : host->m_categories) {
			category.second->forEachTable([&idToSerializedId](rynx::ecs_internal::itable* table) {
				table->for_each_id_field([&idToSerializedId](rynx::id& id) { id.value = idToSerializedId.find(id.value)->second; });
			});
		}
	}

	// serialize everything as-is.
	rynx::serialize(reflections, out); // include reflection of written data
	rynx::serialize(host->m_idCategoryMap.size(), out);

	size_t numNonEmptyCategories = 0;
	for (auto&& category : host->m_categories) {
		if (!category.second->ids().empty())
			++numNonEmptyCategories;
	}
	rynx::serialize(numNonEmptyCategories, out);
	// rynx::serialize(m_categories.size(), out);
	for (auto&& category : host->m_categories) {
		if (category.second->ids().empty())
			continue;

		std::vector<rynx::string> category_typenames;
		category.first.forEachOne([&category, &reflections, &category_typenames](uint64_t type_id) {
			auto* typeReflection = reflections.find(type_id);
			auto* tablePtr = category.second->table_ptr(type_id);
			if (tablePtr) {
				logmsg("%s", tablePtr->type_name().c_str());

				// reflection must exist for each existing table type. otherwise serialization will not work.
				if (tablePtr->can_serialize()) {
					rynx_assert(typeReflection != nullptr, "serializing type that does not have reflection");
				}
			}
			else {
				if (typeReflection) {
					logmsg("no table for type: %s", typeReflection->m_type_name.c_str());
				}
				else {
					logmsg("no table for type %lld", type_id);
				}
			}

			if (typeReflection && typeReflection->m_serialization_allowed)
				category_typenames.emplace_back(typeReflection->m_type_name);
		});

		rynx::serialize(category_typenames, out);

		// for each table in category - serialize table type name, serialize table data
		category.second->forEachTable([&reflections, &out](rynx::ecs_internal::itable* table) {
			// rynx::serialize(table->reflection(reflections).m_type_name);
			logmsg("serializing table %s", table->type_name().c_str());
			table->serialize(out);
		});

		// replace category id vector contents with serialized ids
		std::vector<rynx::ecs::id> idListCopy = category.second->ids();
		for (auto& id : idListCopy) {
			id.value = idToSerializedId[id.value];
		}

		// serialize category id vector
		rynx::serialize(idListCopy, out);
	}

	if constexpr (false) {
		// restore serialized id values back to original run-time id values.
		for (auto&& category : host->m_categories) {
			category.second->forEachTable([&serializedIdToId](rynx::ecs_internal::itable* table) {
				table->for_each_id_field([&serializedIdToId](rynx::id& id) { id.value = serializedIdToId.find(id.value)->second; });
			});
		}
	}
}


rynx::entity_range_t rynx::ecs_detail::raw_serializer::deserialize(rynx::reflection::reflections& reflections, rynx::serialization::vector_reader& in) {

	size_t numEntities;
	size_t numCategories;

	auto id_range_begin = host->m_entities.peek_next_id();

	rynx::reflection::reflections format;
	rynx::deserialize(format, in); // reflection of loaded data
	rynx::deserialize(numEntities, in);
	rynx::deserialize(numCategories, in);

	// construct mapping from serialized ids to ecs ids.
	rynx::unordered_map<uint64_t, uint64_t> serializedIdToEcsId;
	for (size_t i = 1; i <= numEntities; ++i) {
		serializedIdToEcsId[i] = host->m_entities.generateOne();
	}

	for (size_t i = 0; i < numCategories; ++i) {
		auto category_typenames = rynx::deserialize<std::vector<rynx::string>>(in);

		rynx::dynamic_bitset category_id;
		for (auto&& name : category_typenames) {
			auto* typeReflection = reflections.find(name);
			rynx_assert(typeReflection != nullptr, "deserializing unknown type %s", name.c_str());
			category_id.set(typeReflection->m_type_index_value);
		}

		// if category already does not exist - create category.
		if (host->m_categories.find(category_id) == host->m_categories.end()) {
			auto res = host->m_categories.emplace(category_id, rynx::make_unique<rynx::ecs::entity_category>(category_id));
			category_id.forEachOne([&](uint64_t typeId) {
				auto* typeReflection = reflections.find(typeId);
				res.first->second->createNewTable(typeId, typeReflection->m_create_table_func());
			});
		}
		auto category_it = host->m_categories.find(category_id);

		// source category typenames is the correct order to deserialize in.
		std::vector<type_id_t> additional_types;
		for (auto&& name : category_typenames) {
			auto reflection_ptr = reflections.find(name);
			auto type_index_value = reflection_ptr->m_type_index_value;
			
			// TODO: create table func should be stored per type id on top of the ecs (not in category or table, directly in ecs)
			auto* table_ptr = category_it->second->table(type_index_value, reflection_ptr->m_create_table_func);
			if (table_ptr != nullptr) {
				logmsg("deserializing table %s", name.c_str());
				table_ptr->deserialize(in);
				if (table_ptr->is_type_segregated()) {
					// TODO: create map func should be stored per type id on top of the ecs (not in category or table, directly in ecs)
					auto& segregation_map = host->value_segregated_types_map(type_index_value, reflection_ptr->m_create_map_func);
					void* segregated_data_ptr = table_ptr->get(0);
					if (!segregation_map.contains(segregated_data_ptr)) {
						segregation_map.emplace(segregated_data_ptr, host->create_virtual_type());
					}
					auto segregation_virtual_type = segregation_map.get_type_id_for(segregated_data_ptr);
					additional_types.emplace_back(segregation_virtual_type.type_value);
				}
			}
			else {
				logmsg("deser skipping table %s", name.c_str());
			}
		}

		// deserialize category ids and insert them to the category.
		auto categoryIds = rynx::deserialize<std::vector<rynx::ecs::id>>(in);
		for (auto& id : categoryIds) {
			id.value = serializedIdToEcsId.find(id.value)->second;
		}

		auto idCount = category_it->second->m_ids.size();
		category_it->second->m_ids.insert(category_it->second->m_ids.end(), categoryIds.begin(), categoryIds.end());

		// scene serialization does not allow touching the id links.
		if constexpr (false) {
			// restore correct runtime ids to deserialized entities id fields.
			category_it->second->forEachTable([idCount, &serializedIdToEcsId](rynx::ecs_internal::itable* table) {
				table->for_each_id_field_from_index(idCount, [&serializedIdToEcsId](rynx::id& id) {
					id.value = serializedIdToEcsId.find(id.value)->second;
					});
				});
		}

		for (auto& id : categoryIds) {
			host->m_idCategoryMap[id.value] = { category_it->second.get(), index_t(idCount++) };
		}

		// add missing segregation types.
		if (!additional_types.empty()) {
			for (auto&& segregation_virtual_type : additional_types) {
				category_id.set(segregation_virtual_type);
			}

			// if category already does not exist - create category.
			if (host->m_categories.find(category_id) == host->m_categories.end()) {
				// if dst category does not previously exist, we can just move the current category in it's place.
				rynx::dynamic_bitset prev_category_id = std::move(category_it->second->m_types);
				category_it->second->m_types = category_id;
				auto res = host->m_categories.emplace(category_id, std::move(category_it->second));
				host->m_categories.erase(prev_category_id);
			}
			else {
				// TODO: Increase perf by migrating all entities from category at once.
				auto dst_category_it = host->m_categories.find(category_id);
				while (!category_it->second->ids().empty()) {
					dst_category_it->second->migrateEntityFrom(
						category_id,
						category_it->second.get(),
						0,
						host->m_idCategoryMap
					);
				}
				host->m_categories.erase(category_it);
			}
		}
	}

	return { id_range_begin, id_range_begin + numEntities };
}


rynx::serialization::vector_writer rynx::ecs_detail::raw_serializer::serialize(rynx::reflection::reflections& reflections) {
	rynx::serialization::vector_writer out;
	serialize(reflections, out);
	return out;
}
