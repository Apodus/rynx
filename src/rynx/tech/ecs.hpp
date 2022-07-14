#pragma once

#include <rynx/tech/dynamic_bitset.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/type_index.hpp>
#include <rynx/tech/profiling.hpp>
#include <rynx/tech/std/memory.hpp>

#include <rynx/system/assert.hpp>

#include <rynx/tech/serialization_declares.hpp>
#include <rynx/tech/reflection.hpp>
#include <rynx/tech/ecs/table.hpp>

#include <vector>
#include <type_traits>
#include <tuple>
#include <algorithm>
#include <numeric>

namespace rynx {

	template<typename T> struct remove_first_type { using type = std::tuple<>; };
	template<typename T, typename... Ts> struct remove_first_type<std::tuple<T, Ts...>> { using type = std::tuple<Ts...>; };

	template<bool condition, typename TupleType> struct remove_front_if {
		using type = std::conditional_t<condition, typename remove_first_type<TupleType>::type, TupleType>;
	};

	template<typename T, size_t n> struct remove_first_n_type { using type = void; };
	template<typename... Ts, size_t n> struct remove_first_n_type<std::tuple<Ts...>, n> { using type = typename remove_first_n_type<typename remove_first_type<std::tuple<Ts...>>::type, n - 1>::type; };
	template<typename... Ts> struct remove_first_n_type<std::tuple<Ts...>, 0> { using type = std::tuple<Ts...>; };

	template<typename T, typename... Others> constexpr bool isAny() { return false || (std::is_same_v<std::remove_reference_t<T>, std::remove_reference_t<Others>> || ...); }
	template<typename T, typename... Ts> constexpr T& first_of(T&& t, Ts&&...) { return t; }

	class ecs {
	public:
		struct range {
			index_t begin = 0;
			index_t step = 0;
		};

		class entity_category;

	private:
		enum class DataAccess {
			Mutable,
			Const
		};


		// TODO: Use some actual hash that works better.
		struct bitset_hash {
			size_t operator()(const dynamic_bitset& v) const {
				const auto& buffer = v.data();
				size_t hash_value = 0x783abb8ddead7175;
				for (size_t i = 0; i < buffer.size(); ++i) {
					hash_value ^= buffer[i] >> 13;
					hash_value ^= buffer[i] << 15;
				}
				return hash_value;
			}
		};

		rynx::ecs_internal::entity_index m_entities;

		rynx::unordered_map<entity_id_t, std::pair<entity_category*, index_t>> m_idCategoryMap;
		rynx::unordered_map<dynamic_bitset, rynx::unique_ptr<entity_category>, bitset_hash> m_categories;
		rynx::unordered_map<type_id_t, opaque_unique_ptr<rynx::ecs_internal::ivalue_segregation_map>> m_value_segregated_types_maps;
		std::vector<type_id_t> m_virtual_types_released;

		auto& categories() { return m_categories; }
		const auto& categories() const { return m_categories; }

		rynx::ecs_internal::ivalue_segregation_map& value_segregated_types_map(type_id_t type_id, rynx::function<rynx::unique_ptr<rynx::ecs_internal::ivalue_segregation_map>()> map_create_func) {
			auto it = m_value_segregated_types_maps.find(type_id);
			if (it == m_value_segregated_types_maps.end()) {
				it = m_value_segregated_types_maps.emplace(type_id, map_create_func()).first;
			}
			return *it->second.get();
		}

		template<typename T>
		auto& value_segregated_types_map() {
			using map_type = rynx::ecs_internal::value_segregation_map<T>;
			
			auto type_id = rynx::type_index::id<T>();
			auto it = m_value_segregated_types_maps.find(type_id);
			if (it == m_value_segregated_types_maps.end()) {
				opaque_unique_ptr<rynx::ecs_internal::ivalue_segregation_map> map_ptr(new map_type(), [](void* v) {
					delete static_cast<map_type*>(v);
				});
				it = m_value_segregated_types_maps.emplace(type_id, std::move(map_ptr)).first;
			}
			return *static_cast<map_type*>(it->second.get());
		}

	public:
		using id = rynx::id;

		class entity_category {
		public:
			entity_category(dynamic_bitset types) : m_types(std::move(types)) {}

			bool includesAll(const dynamic_bitset& types) const { return m_types.includes(types); }
			bool includesNone(const dynamic_bitset& types) const { return m_types.excludes(types); }

			void createNewTable(uint64_t typeId, rynx::unique_ptr<rynx::ecs_internal::itable> tablePtr) {
				if (typeId >= m_tables.size()) {
					m_tables.resize(((3 * typeId) >> 1) + 1);
				}
				rynx_assert(m_tables[typeId] == nullptr, "should never replace existing tables");
				m_tables[typeId] = std::move(tablePtr);
			}

			void erase(index_t index, rynx::unordered_map<entity_id_t, std::pair<entity_category*, index_t>>& idmap) {
				for (auto&& table : m_tables) {
					if(table)
						table->erase(index);
				}

				rynx_assert(index < m_ids.size(), "out of bounds");
				auto erasedEntityId = m_ids[index];
				m_ids[index] = std::move(m_ids.back());
				
				// update ecs-wide id<->category mapping
				idmap.find(m_ids[index].value)->second = { this, index };
				idmap.erase(erasedEntityId.value);
				
				m_ids.pop_back();
			}

			// TODO: Rename better. This is like bubble-sort single step.
			// TODO: Use some smarter algorithm?
			template<typename T> void sort_one_step(rynx::unordered_map<entity_id_t, std::pair<entity_category*, index_t>>& idmap) {
				auto type_index_of_t = rynx::type_index::id<T>();
				auto& table_t = table<T>(type_index_of_t);
				
				std::vector<index_t> sequential_swaps; // for each index in vector, swap index <-> index+1
				for (index_t table_index = 1; table_index < table_t.size(); ++table_index) {
					if (table_t[table_index - 1] > table_t[table_index]) {
						sequential_swaps.emplace_back(table_index - 1);
						
						// swap content of T and index mapping.
						std::swap(table_t[table_index - 1], table_t[table_index]);
						
						std::swap(m_ids[table_index - 1], m_ids[table_index]);
						idmap.find(m_ids[table_index - 1])->second.second = table_index - 1;
						idmap.find(m_ids[table_index])->second.second = table_index;
					}
				}

				for (type_id_t index = 0; index < m_tables.size(); ++index) {
					auto& table_ptr = m_tables[index];
					if (table_ptr && index != type_index_of_t) {
						table_ptr->swap_adjacent_indices_for(sequential_swaps);
					}
				}
			}

			template<typename T> void sort(rynx::unordered_map<entity_id_t, std::pair<entity_category*, index_t>>& idmap) {
				auto type_index_of_t = rynx::type_index::id<T>();
				auto& table_t = table<T>(type_index_of_t);

				std::vector<index_t> relative_sort_order(m_ids.size());
				std::iota(relative_sort_order.begin(), relative_sort_order.end(), 0);
				std::sort(relative_sort_order.begin(), relative_sort_order.end(),
					[&](index_t a, index_t b) {
						return table_t[a] < table_t[b];
					}
				);
				
				for (auto& table_ptr : m_tables) {
					if (table_ptr) {
						table_ptr->swap_to_given_order(relative_sort_order);
					}
				}

				{
					std::vector<index_t> current_index_to_original_index(m_ids.size());
					std::vector<index_t> original_index_to_current_index(m_ids.size());

					std::iota(current_index_to_original_index.begin(), current_index_to_original_index.end(), 0);
					std::iota(original_index_to_current_index.begin(), original_index_to_current_index.end(), 0);

					for (index_t i = 0, size = index_t(m_ids.size()); i < size; ++i) {
						index_t mapped_location = original_index_to_current_index[relative_sort_order[i]];
						if (mapped_location != i) {
							idmap.find(m_ids[i].value)->second.second = mapped_location;
							idmap.find(m_ids[mapped_location].value)->second.second = index_t(i);
							std::swap(m_ids[i], m_ids[mapped_location]);

							std::swap(current_index_to_original_index[i], current_index_to_original_index[mapped_location]);
							std::swap(
								original_index_to_current_index[current_index_to_original_index[i]],
								original_index_to_current_index[current_index_to_original_index[mapped_location]]
							);
						}
					}
				}
			}


		private:
			// private implementation details for insertion operations.
			// virtual types are not added because they hold no data by definition.
			template<typename T> void insertSingleComponent([[maybe_unused]] const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, [[maybe_unused]] T&& component) {
				if constexpr (!std::is_same_v<std::remove_cvref_t<T>, rynx::type_index::virtual_type> && !std::is_empty_v<std::remove_cvref_t<T>>) {
					table<T>(typeAliases).emplace_back(std::forward<T>(component));
				}
			}

			template<typename T> void insertSingleComponentVector(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, std::vector<T>& component) {
				if constexpr (!std::is_same_v<std::remove_cvref_t<T>, rynx::type_index::virtual_type>) {
					table<T>(typeAliases).insert(component);
				}
			}

		public:
			template<typename...Components> size_t insertNew(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, const std::vector<entity_id_t>& ids, std::vector<Components>& ... components) {
				(insertSingleComponentVector(typeAliases, components), ...);
				size_t index = m_ids.size();
				m_ids.insert(m_ids.end(), ids.begin(), ids.end());
				return index;
			}

			void insertComponent_typeErased(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, type_id_t type_id, rynx::ecs_table_create_func& creator, opaque_unique_ptr<void> data) {
				auto it = typeAliases.find(type_id);
				if (it == typeAliases.end()) {
					auto* table_ptr = table(type_id, creator);
					if(table_ptr != nullptr)
						table_ptr->insert(std::move(data));
				}
				else {
					auto* table_ptr = table(it->second, creator);
					if (table_ptr != nullptr)
						table_ptr->insert(std::move(data));
				}
			}

			template<typename...Components> void insertComponents(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, Components&& ... components) {
				(insertSingleComponent(typeAliases, std::forward<Components>(components)), ...);
			}

			template<typename...Components> size_t insertNew(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, entity_id_t id, Components&& ... components) {
				insertComponents(typeAliases, std::forward<Components>(components)...);
				size_t index = m_ids.size();
				m_ids.emplace_back(id);
				return index;
			}

			template<typename Component> void eraseComponentFromIndex([[maybe_unused]] const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, [[maybe_unused]] index_t index) {
				if constexpr (!std::is_empty_v<std::remove_cvref_t<Component>> && !std::is_base_of_v<rynx::ecs_value_segregated_component_tag, std::remove_cvref_t<Component>>) {
					auto& t = table<Component>(typeAliases);
					t[index] = std::move(t.back());
					t.pop_back();
				}
			}

			void eraseComponentFromIndex([[maybe_unused]] const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, index_t index, type_id_t type_id_v) {
				auto it = typeAliases.find(type_id_v);
				if (it != typeAliases.end())
					type_id_v = it->second;
				
				auto& t = table(static_cast<int32_t>(type_id_v));
				t.replace_with_back_and_pop(index);
			}

			template<typename...Components> void eraseComponentsFromIndex(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, index_t index) { (eraseComponentFromIndex<Components>(typeAliases, index), ...); }

			void copyTypesFrom(entity_category* other, const dynamic_bitset& types) {
				type_id_t typeId = types.nextOne(0);
				while (typeId != dynamic_bitset::npos) {
					// TODO: Get rid of null check :( it is guarding against virtual types and tag types.
					if (typeId < other->m_tables.size() && other->m_tables[typeId]) {
						other->m_tables[typeId]->copyTableTypeTo(typeId, m_tables);
					}
					typeId = types.nextOne(typeId + 1);
				}
			}
			void copyTypesTo(entity_category* other, const dynamic_bitset& types) { other->copyTypesFrom(this, types); }

			void copyTypesFrom(entity_category* other) {
				for (index_t type_id = 0; type_id < other->m_tables.size(); ++type_id) {
					if (other->m_tables[type_id]) {
						other->m_tables[type_id]->copyTableTypeTo(type_id, m_tables);
					}
				}
			}
			void copyTypesTo(entity_category* other) { other->copyTypesFrom(this); }

			size_t size() const { return m_ids.size(); }

			std::vector<id>& ids() { return m_ids; }
			const std::vector<id>& ids() const { return m_ids; }

			template<DataAccess accessType, typename ComponentsTuple> auto tables() {
				return getTables<accessType, ComponentsTuple>()(*this);
			}

			template<DataAccess accessType, typename ComponentsTuple> auto table_datas(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases) {
				return getTableDatas<accessType, ComponentsTuple>()(*this, typeAliases);
			}

			template<DataAccess accessType, typename T> auto* table_data(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases) {
				return getTableData<accessType, T>()(*this, typeAliases);
			}

			const dynamic_bitset& types() const { return m_types; }

			void migrateEntityFrom(
				const dynamic_bitset& types,
				entity_category* source,
				index_t source_index,
				rynx::unordered_map<entity_id_t, std::pair<entity_category*, index_t>>& idmap
			) {
				type_id_t typeId = types.nextOne(0);
				while (typeId != dynamic_bitset::npos) {
					// virtual types are present in bitset, but do not have a table.
					// TODO: mark virtual types in a separate bitset or something and compare against that. this nullcheck can hide actual bugs.
					if (typeId < m_tables.size() && m_tables[typeId]) {
						source->m_tables[typeId]->moveFromIndexTo(source_index, m_tables[typeId].get());
					}
					typeId = types.nextOne(typeId + 1);
				}

				m_ids.emplace_back(source->m_ids[source_index]);
				
				rynx_assert(source->m_ids.size() > source_index, "out of bounds");
				source->m_ids[source_index] = source->m_ids.back();
				
				// update mapping
				idmap.find(source->m_ids[source_index].value)->second = { source, index_t(source_index) };
				source->m_ids.pop_back();
				
				idmap.find(m_ids.back().value)->second = { this, index_t(m_ids.size() - 1) };
			}

			template<typename T, typename U = std::remove_const_t<std::remove_reference_t<T>>> auto& table(type_id_t typeIndex) {
				if (typeIndex >= m_tables.size()) {
					m_tables.resize(((3 * typeIndex) >> 1) + 1);
				}
				if (!m_tables[typeIndex]) {
					if constexpr (std::is_empty_v<U>) {
						rynx_assert(false, "never create tag tables");
					}
					else {
						m_tables[typeIndex] = rynx::make_unique<rynx::ecs_internal::component_table<U>>(typeIndex);
					}
				}
				
				if constexpr (std::is_empty_v<U>) {
					rynx_assert(false, "never create tag tables");
					return *static_cast<rynx::ecs_internal::component_table<U>*>(nullptr);
				}
				else {
					return *static_cast<rynx::ecs_internal::component_table<U>*>(m_tables[typeIndex].get());
				}
			}

			template<typename T, typename U = std::remove_const_t<std::remove_reference_t<T>>> auto& table() {
				type_id_t typeIndex = rynx::type_index::id<U>();
				return table<U>(typeIndex);
			}

			template<typename T> auto& table(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases) {
				type_id_t original = rynx::type_index::id<T>();
				auto it = typeAliases.find(original);
				if (it == typeAliases.end()) {
					return this->table<T>(original);
				}
				return this->table<T>(it->second);
			}

			template<typename T> const auto& table(type_id_t typeIndex) const { return const_cast<entity_category*>(this)->table<T>(typeIndex); }
			template<typename T> const auto& table() const { return const_cast<entity_category*>(this)->table<T>(); }
			template<typename T> const auto& table(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases) const { return const_cast<entity_category*>(this)->table<T>(typeAliases); }

			rynx::ecs_internal::itable* table_ptr(type_id_t type_index_value) {
				if (type_index_value < m_tables.size()) {
					return m_tables[type_index_value].get();
				}
				return nullptr;
			}

			rynx::ecs_internal::itable& table(type_id_t type_index_value) {
				rynx_assert(m_tables[type_index_value] != nullptr, "table must exist");
				return *m_tables[type_index_value];
			}

			// returning pointer type to highlight the fact that calling this may return nullptr (tag types).
			rynx::ecs_internal::itable* table(type_id_t type_index_value, rynx::ecs_table_create_func& creator) {
				if (type_index_value >= m_tables.size()) {
					m_tables.resize(2 * (type_index_value + 1));
				}
				if (m_tables[type_index_value] == nullptr) {
					m_tables[type_index_value] = creator();
				}
				return m_tables[type_index_value].get();
			}

			// this is not particularly fast - but required for some editor related things for example.
			template<typename Func> void forEachTable(Func&& func) {
				for (auto&& table : m_tables) {
					if (table) {
						func(table.get());
					}
				}
			}

		private:
			template<DataAccess, typename Ts> struct getTables {};
			template<DataAccess accessType, typename... Ts> struct getTables<accessType, std::tuple<Ts...>> {
				[[nodiscard]] auto operator()(entity_category& categorySource) {
					if constexpr (accessType == DataAccess::Const) {
						return std::make_tuple(&const_cast<const entity_category&>(categorySource).table<Ts>()...);
					}
					else {
						return std::make_tuple(&categorySource.table<Ts>()...);
					}
				}
			};

			template<DataAccess, typename Ts> struct getTableDatas {};
			template<DataAccess accessType, typename... Ts> struct getTableDatas<accessType, std::tuple<Ts...>> {
				[[nodiscard]] auto operator()(entity_category& categorySource, const rynx::unordered_map<type_id_t, type_id_t>& typeAliases) {
					if constexpr (accessType == DataAccess::Const) {
						return std::make_tuple(const_cast<const entity_category&>(categorySource).table<Ts>(typeAliases).data()...);
					}
					else {
						return std::make_tuple(categorySource.table<Ts>(typeAliases).data()...);
					}
				}
			};

			template<DataAccess accessType, typename T> struct getTableData {
				[[nodiscard]] auto operator()(entity_category& categorySource, const rynx::unordered_map<type_id_t, type_id_t>& typeAliases) {
					if constexpr (accessType == DataAccess::Const) {
						return const_cast<const entity_category&>(categorySource).table<T>(typeAliases).data();
					}
					else {
						return categorySource.table<T>(typeAliases).data();
					}
				}
			};

			friend class rynx::ecs; // TODO: Remove

			dynamic_bitset m_types;
			std::vector<rynx::unique_ptr<rynx::ecs_internal::itable>> m_tables;
			std::vector<id> m_ids;
		};

		// TODO: is is ok to have the parallel implementation baked into iterator?
		
		template<typename category_source>
		class gatherer {
		public:
			gatherer(category_source& ecs) : m_ecs(ecs) {}
			gatherer(const category_source& ecs) : m_ecs(const_cast<category_source&>(ecs)) {}

			gatherer& include(dynamic_bitset inTypes) { includeTypes = std::move(inTypes); return *this; }
			gatherer& exclude(dynamic_bitset notInTypes) { excludeTypes = std::move(notInTypes); return *this; }

			void ids(std::vector<rynx::ecs::id>& out) const {
				auto& categories = m_ecs.categories();
				for (auto&& entity_category : categories) {
					if (entity_category.second->includesAll(includeTypes) & entity_category.second->includesNone(excludeTypes)) {
						auto& ids = entity_category.second->ids();
						out.insert(out.end(), ids.begin(), ids.end());
					}
				}
			}

			index_t count() const {
				index_t result = 0;
				auto& categories = m_ecs.categories();
				for (auto&& entity_category : categories) {
					if (entity_category.second->includesAll(includeTypes) & entity_category.second->includesNone(excludeTypes)) {
						result += static_cast<index_t>(entity_category.second->ids().size());
					}
				}
				return result;
			}

			gatherer& type_aliases(rynx::unordered_map<type_id_t, type_id_t>&& typeAliases) {
				m_typeAliases = std::move(typeAliases);
				return *this;
			}

			type_id_t use_mapped_type_if_present(type_id_t v) {
				auto it = m_typeAliases.find(v);
				if (it != m_typeAliases.end()) {
					return it->second;
				}
				return v;
			}
		private:
			template<typename T> void unpack_types_single(rynx::dynamic_bitset& dst) {
				dst.set(
					use_mapped_type_if_present(
						rynx::type_index::id<T>()
					)
				);
			}

		protected:
			template<typename TupleType, size_t...Is> void unpack_types(std::index_sequence<Is...>) {
				(unpack_types_single<typename std::tuple_element<Is, TupleType>::type>(includeTypes), ...);
			}
			
			template<typename... Ts> void unpack_types() {
				(unpack_types_single<Ts>(includeTypes), ...);
			}

			template<typename TupleType, size_t...Is> void unpack_types_exclude(std::index_sequence<Is...>) {
				(unpack_types_single<typename std::tuple_element<Is, TupleType>::type>(excludeTypes), ...);
			}
			
			template<typename... Ts> void unpack_types_exclude() {
				(unpack_types_single<Ts>(excludeTypes), ...);
			}

			dynamic_bitset includeTypes;
			dynamic_bitset excludeTypes;
			rynx::unordered_map<type_id_t, type_id_t> m_typeAliases;
			category_source& m_ecs;
		};

		template<typename category_source>
		class sorter : public gatherer<category_source> {
		public:
			sorter(category_source& ecs) : gatherer<category_source>(ecs) {}
			sorter(const category_source& ecs) : gatherer<category_source>(ecs) {}

			template<typename... Args> sorter& in() { unpack_types<Args...>(); return *this; }
			template<typename... Args> sorter& notIn() { unpack_types_exclude<Args...>(); return *this; }

			sorter& in(rynx::type_index::virtual_type t) { this->includeTypes.set(t.type_value); return *this; }
			sorter& notIn(rynx::type_index::virtual_type t) { this->excludeTypes.set(t.type_value); return *this; }

			template<typename T> void sort_categories_by() {
				auto& categories = this->m_ecs.categories();
				this->template unpack_types<T>();
				for (auto&& entity_category : categories) {
					if (entity_category.second->includesAll(this->includeTypes) &
						entity_category.second->includesNone(this->excludeTypes))
					{
						entity_category.second->template sort<T>(this->m_ecs.entity_category_map());
					}
				}
			}

			template<typename T> void sort_buckets_one_step() {
				auto& categories = this->m_ecs.categories();
				this->template unpack_types<T>();
				for (auto&& entity_category : categories) {
					if (entity_category.second->includesAll(this->includeTypes) &
						entity_category.second->includesNone(this->excludeTypes))
					{
						auto& ids = entity_category.second->ids();
						entity_category.second->template sort_one_step<T>(this->m_ecs.entity_category_map());
					}
				}
			}
		};

		template<DataAccess accessType, typename category_source, typename F> class buffer_iterator {};
		template<DataAccess accessType, typename category_source, typename Ret, typename Class, typename FArg, typename... Args>
		class buffer_iterator<accessType, category_source, Ret(Class::*)(size_t, FArg, Args...) const> : public gatherer<category_source> {
		public:
			buffer_iterator(const category_source& ecs) : gatherer<category_source>(ecs) {
				this->m_ecs.template componentTypesAllowed<std::add_rvalue_reference_t<std::remove_pointer_t<Args>>...>();
				static_assert(!std::is_same_v<FArg, rynx::ecs::id*>, "id pointer must be const when iterating buffers!");
				if constexpr (!std::is_same_v<FArg, rynx::ecs::id const*>) {
					this->m_ecs.template componentTypesAllowed<std::add_rvalue_reference_t<std::remove_pointer_t<FArg>>>();
				}
			}

			template<typename F> void for_each_buffer(F&& op) {
				constexpr bool is_id_query = std::is_same_v<FArg, rynx::ecs::id const *>;

				using types_t = std::tuple<
					std::remove_const_t<std::remove_pointer_t<FArg>>,
					std::remove_const_t<std::remove_pointer_t<Args>>...
				>;
				using id_types_t = typename rynx::remove_first_type<types_t>::type;

				if constexpr (is_id_query) {
					this->template unpack_types<id_types_t>(std::make_index_sequence<std::tuple_size<id_types_t>::value>());
				}
				else {
					this->template unpack_types<types_t>(std::make_index_sequence<std::tuple_size<types_t>::value>());
				}

				auto& categories = this->m_ecs.categories();
				for (auto&& entity_category : categories) {
					if (entity_category.second->includesAll(this->includeTypes) & entity_category.second->includesNone(this->excludeTypes)) {
						auto& ids = entity_category.second->ids();
						if constexpr (is_id_query) {
							auto call_user_op = [size = ids.size(), ids_data = ids.data(), &op](auto*... args) {
								op(size, ids_data, args...);
							};
							std::apply(call_user_op, entity_category.second->template table_datas<accessType, id_types_t>(this->m_typeAliases));
						}
						else {
							auto call_user_op = [size = ids.size(), &op](auto... args) {
								op(size, args...);
							};
							std::apply(call_user_op, entity_category.second->template table_datas<accessType, types_t>(this->m_typeAliases));
						}
					}
				}
			}
		};

		template<DataAccess accessType, typename category_source, typename F> class entity_iterator {};
		template<DataAccess accessType, typename category_source, typename Ret, typename Class, typename FArg, typename... Args>
		class entity_iterator<accessType, category_source, Ret(Class::*)(FArg, Args...) const> : public gatherer<category_source> {
		public:
			entity_iterator(category_source& ecs) : gatherer<category_source>(ecs) {
				this->m_ecs.template componentTypesAllowed<Args...>();
				if constexpr (!std::is_same_v<FArg, rynx::ecs::id>) {
					this->m_ecs.template componentTypesAllowed<FArg>();
				}
			}
			
			entity_iterator(const category_source& ecs) : gatherer<category_source>(ecs) {
				this->m_ecs.template componentTypesAllowed<Args...>();
				if constexpr (!std::is_same_v<FArg, rynx::ecs::id>) {
					this->m_ecs.template componentTypesAllowed<FArg>();
				}
			}

			template<typename F> std::vector<id> ids_if(F&& op) {
				using components_t = std::tuple<FArg, Args...>;
				using components_without_first_t = std::tuple<Args...>;

				constexpr bool is_id_query = std::is_same_v<std::tuple_element_t<0, components_t>, rynx::ecs::id>;
				if constexpr (!is_id_query) {
					this->template unpack_types<components_t>(std::index_sequence_for<components_t>());
				}
				else {
					this->template unpack_types<components_without_first_t>(std::index_sequence_for<components_without_first_t>());
				}

				std::vector<id> gathered_ids;
				auto& categories = this->m_ecs.categories();
				for (auto&& entity_category : categories) {
					if (entity_category.second->includesAll(this->includeTypes) & entity_category.second->includesNone(this->excludeTypes)) {
						auto& ids = entity_category.second->ids();
						auto* id_begin = ids.data();
						auto* id_end = id_begin + ids.size();
						
						if constexpr (is_id_query) {
							auto parameters_tuple = entity_category.second->template table_datas<accessType, components_without_first_t>(this->m_typeAliases);
							auto iteration_func = [&op, id_begin, id_end, &gathered_ids](auto*... ptrs) mutable {
								while (id_begin != id_end) {
									if (op(*id_begin, *ptrs...)) {
										gathered_ids.emplace_back(*id_begin);
									}
									(++ptrs, ...);
									++id_begin;
								}
							};
							std::apply(iteration_func, parameters_tuple);
						}
						else {
							auto parameters_tuple = entity_category.second->template table_datas<accessType, components_t>(this->m_typeAliases);
							auto iteration_func = [&op, id_begin, id_end, &gathered_ids](auto*... ptrs) mutable {
								while (id_begin != id_end) {
									if (op(*ptrs...)) {
										gathered_ids.emplace_back(*id_begin);
									}
									(++ptrs, ...);
									++id_begin;
								}
							};
							std::apply(iteration_func, parameters_tuple);
						}
					}
				}
				return gathered_ids;
			}

			template<typename F> void for_each(F&& op) {
				constexpr bool is_id_query = std::is_same_v<FArg, rynx::ecs::id>;
				if constexpr (!is_id_query) {
					this->template unpack_types<FArg>();
				}
				this->template unpack_types<Args...>();
				
				auto& categories = this->m_ecs.categories();
				for (auto&& entity_category : categories) {
					if (entity_category.second->includesAll(this->includeTypes) & entity_category.second->includesNone(this->excludeTypes)) {
						auto& ids = entity_category.second->ids();
						if constexpr (is_id_query) {
							call_user_op<is_id_query>(std::forward<F>(op), ids, entity_category.second->template table_data<accessType, Args>(this->m_typeAliases)...);
						}
						else {
							call_user_op<is_id_query>(std::forward<F>(op), ids, entity_category.second->template table_data<accessType, FArg>(this->m_typeAliases), entity_category.second->template table_data<accessType, Args>(this->m_typeAliases)...);
						}
					}
				}
			}

			template<typename F> void for_each_partial(rynx::ecs::range r, F&& op) {
				constexpr bool is_id_query = std::is_same_v<FArg, rynx::ecs::id>;
				if constexpr (!is_id_query) {
					this->template unpack_types<FArg>();
				}
				this->template unpack_types<Args...>();

				index_t current_index = 0;

				auto& categories = this->m_ecs.categories();
				for (auto&& entity_category : categories) {
					if (entity_category.second->includesAll(this->includeTypes) & entity_category.second->includesNone(this->excludeTypes)) {
						auto& ids = entity_category.second->ids();
						auto ids_size = index_t(ids.size());

						if ((current_index + ids_size <= r.begin) | (current_index >= r.begin + r.step)) {
							current_index += ids_size;
							continue;
						}

						rynx::ecs::range category_range;

						if (r.begin < current_index) {
							category_range.begin = 0;
							index_t step_offset = current_index % r.step;
							category_range.step = (category_range.begin + r.step - step_offset <= ids_size) ? r.step - step_offset : ids_size - category_range.begin;
						}
						else {
							category_range.begin = r.begin - current_index;
							category_range.step = (category_range.begin + r.step <= ids_size) ? r.step : ids_size - category_range.begin;
						}

						rynx_assert(category_range.begin < ids_size, "hehe");
						rynx_assert(category_range.step <= ids_size, "hehe");
						current_index += ids_size;

						if constexpr (is_id_query) {
							call_user_op_range<is_id_query>(category_range, std::forward<F>(op), ids, entity_category.second->template table_data<accessType, Args>(this->m_typeAliases)...);
						}
						else {
							call_user_op_range<is_id_query>(category_range, std::forward<F>(op), ids, entity_category.second->template table_data<accessType, FArg>(this->m_typeAliases), entity_category.second->template table_data<accessType, Args>(this->m_typeAliases)...);
						}
					}
				}
			}

			template<typename F, typename TaskContext> auto for_each_parallel(TaskContext&& task_context, F&& op) {
				constexpr bool is_id_query = std::is_same_v<FArg, rynx::ecs::id>;
				if constexpr (!is_id_query) {
					this->template unpack_types<FArg>();
				}
				this->template unpack_types<Args...>();

				auto parallel_ops_container = task_context.parallel();
				
				auto& categories = this->m_ecs.categories();
				for (auto&& entity_category : categories) {
					if (entity_category.second->includesAll(this->includeTypes) & entity_category.second->includesNone(this->excludeTypes)) {
						auto& ids = entity_category.second->ids();
						if constexpr (is_id_query) {
							call_user_op_parallel<is_id_query>(std::forward<F>(op), parallel_ops_container, ids, entity_category.second->template table_data<accessType, Args>(this->m_typeAliases)...);
						}
						else {
							call_user_op_parallel<is_id_query>(std::forward<F>(op), parallel_ops_container, ids, entity_category.second->template table_data<accessType, FArg>(this->m_typeAliases), entity_category.second->template table_data<accessType, Args>(this->m_typeAliases)...);
						}
					}
				}

				return parallel_ops_container;
			}

		private:
			template<bool isIdQuery, typename F, typename... Ts>
			static void call_user_op(F&& op, std::vector<id>& ids, Ts* rynx_restrict ... data_ptrs) {
				auto ids_size = ids.size();
				if constexpr (isIdQuery) {
					std::for_each(ids.data(), ids.data() + ids_size, [=](const id entityId) mutable {
						op(entityId, (*data_ptrs++)...);
					});
				}
				else {
					for (size_t i = 0; i < ids_size; ++i) {
						op((*data_ptrs++)...);
					}
				}
				rynx_assert(ids.size() == ids_size, "creating/deleting entities is not allowed during iteration.");
			}

			template<bool isIdQuery, typename F, typename... Ts>
			static void call_user_op_range(rynx::ecs::range iteration_range, F&& op, std::vector<id>& ids, Ts* rynx_restrict ... data_ptrs) {
				[[maybe_unused]] auto ids_size = ids.size();
				((data_ptrs += iteration_range.begin), ...);
				if constexpr (isIdQuery) {
					std::for_each(ids.data() + iteration_range.begin, ids.data() + (iteration_range.begin + iteration_range.step), [=](const id entityId) mutable {
						op(entityId, (*data_ptrs++)...);
					});
				}
				else {
					// TODO: optimize for case sizeof...(data_ptrs) > 0, iterating an extra counter "i" is unnecessary
					for (size_t i = 0, size_ = iteration_range.step; i < size_; ++i) {
						op((*data_ptrs++)...);
					}
				}
				rynx_assert(ids.size() == ids_size, "creating/deleting entities is not allowed during iteration.");
			}

			template<bool isIdQuery, typename F, typename ParallelOp, typename... Ts>
			static void call_user_op_parallel(F&& op, ParallelOp&& parallel_ops_container, std::vector<id>& ids, Ts* rynx_restrict ... data_ptrs) {
				if constexpr (isIdQuery) {
					parallel_ops_container
						.range(0, ids.size())
						.execute([op, &ids, args = std::make_tuple(data_ptrs...)](int64_t index) mutable {
						std::apply([&ids, op, index](auto*... ptrs) mutable {
							op(ids[index], ptrs[index]...);
						}, args);
					});
				}
				else {
					parallel_ops_container
						.range(0, ids.size())
						.execute([op, args = std::make_tuple(data_ptrs...)](int64_t index) mutable {
						std::apply([op, index](auto*... ptrs) mutable {
							op(ptrs[index]...);
						}, args);
					});
				}
			}
		};

		// support for mutable lambdas / non-const member functions
		template<DataAccess accessType, typename category_source, typename Ret, typename Class, typename FArg, typename... Args>
		class entity_iterator<accessType, category_source, Ret(Class::*)(FArg, Args...)> : public entity_iterator<accessType, category_source, Ret(Class::*)(FArg, Args...) const> {
		public:
			entity_iterator(category_source& ecs) : entity_iterator<accessType, category_source, Ret(Class::*)(FArg, Args...) const>(ecs) {}
			entity_iterator(const category_source& ecs) : entity_iterator<accessType, category_source, Ret(Class::*)(FArg, Args...) const>(ecs) {}
		};


		// TODO: don't use boolean template parameter. use enum class instead.
		template<typename category_source, bool allowEditing = false>
		class entity {
		private:
			void update_category_and_index() {
				auto categoryAndIndex = categorySource->category_and_index_for(m_id);
				m_entity_category = categoryAndIndex.first;
				m_category_index = categoryAndIndex.second;
				rynx_assert(m_entity_category->ids().size() > m_category_index && m_entity_category->ids()[m_category_index] == m_id, "entity mapping is broken");
			}

		public:
			entity(category_source& parent, entity_id_t id) : categorySource(&parent), m_id(id) {
				update_category_and_index();
			}

			entity(const entity& other) = default;

			std::vector<rynx::reflection::type> reflections(rynx::reflection::reflections& reflections) const {
				std::vector<rynx::reflection::type> result;
				m_entity_category->forEachTable([&result, &reflections](rynx::ecs_internal::itable* tbl) {
					auto* reflection_entry = reflections.find(tbl->m_type_id);
					if (reflection_entry) {
						result.emplace_back(*reflection_entry);
					}
					else {
						logmsg("WARNING: reflections undefined for type: '%s' which is used as an ecs component", tbl->type_name().c_str());
					}
				});
				return result;
			}

			// for editor use only.
			void* get(int32_t type_index_value) {
				return m_entity_category->table(type_index_value).get(m_category_index);
			}

			template<typename T> T& get() {
				categorySource->template componentTypesAllowed<T&>();
				rynx_assert(has<T>(), "requested component type not in entity");
				rynx_assert(m_entity_category->ids().size() > m_category_index && m_entity_category->ids()[m_category_index] == m_id, "entity mapping is broken");
				return m_entity_category->template table<T>()[m_category_index];
			}

			template<typename T> T& get(rynx::type_index::virtual_type type) {
				categorySource->template componentTypesAllowed<T&>();
				rynx_assert(has<T>(), "requested component type not in entity");
				rynx_assert(m_entity_category->ids().size() > m_category_index && m_entity_category->ids()[m_category_index] == m_id, "entity mapping is broken");
				return m_entity_category->template table<T>(type.type_value)[m_category_index];
			}

			template<typename T> T* try_get() {
				categorySource->template componentTypesAllowed<T&>();
				type_id_t typeIndex = rynx::type_index::id<T>();
				rynx_assert(m_entity_category != nullptr, "referenced entity seems to not exist.");
				rynx_assert(m_entity_category->ids().size() > m_category_index && m_entity_category->ids()[m_category_index] == m_id, "entity mapping is broken");
				if (m_entity_category->types().test(typeIndex)) {
					return &m_entity_category->template table<T>(typeIndex)[m_category_index];
				}
				return nullptr;
			}

			template<typename T> T* try_get(rynx::type_index::virtual_type type) {
				categorySource->template componentTypesAllowed<T&>();
				type_id_t typeIndex = type.type_value;
				rynx_assert(m_entity_category != nullptr, "referenced entity seems to not exist.");
				rynx_assert(m_entity_category->ids().size() > m_category_index && m_entity_category->ids()[m_category_index] == m_id, "entity mapping is broken");
				if (m_entity_category->types().test(typeIndex)) {
					return &m_entity_category->template table<T>(typeIndex)[m_category_index];
				}
				return nullptr;
			}

			template<typename... Ts> bool has() const noexcept {
				categorySource->template componentTypesAllowed<std::add_const_t<Ts> ...>();
				rynx_assert(m_entity_category, "referenced entity seems to not exist.");
				rynx_assert(m_entity_category->ids().size() > m_category_index && m_entity_category->ids()[m_category_index] == m_id, "entity mapping is broken");
				return true & (m_entity_category->types().test(rynx::type_index::id<Ts>()) & ...);
			}

			bool has(type_id_t t) const noexcept {
				rynx_assert(m_entity_category, "referenced entity seems to not exist.");
				return m_entity_category->types().test(t);
			}

			bool has(rynx::type_index::virtual_type t) const noexcept {
				rynx_assert(m_entity_category, "referenced entity seems to not exist.");
				return m_entity_category->types().test(t.type_value);
			}

			template<typename T>
			bool has(rynx::type_index::virtual_type t) const noexcept {
				return has(t);
			}

			template<typename... Ts> void remove() {
				static_assert(allowEditing && sizeof...(Ts) >= 0, "You took this entity from an ecs::view. Removing components like this is not allowed. Use edit_view to remove components instead.");
				categorySource->editor().template removeFromEntity<Ts...>(m_id);
				update_category_and_index();
			}

			template<typename T> void remove(rynx::type_index::virtual_type type) {
				static_assert(allowEditing && sizeof(T) >= 0, "You took this entity from an ecs::view. Removing components like this is not allowed. Use edit_view to remove components instead.");
				categorySource->editor().template type_alias<T>(type).template removeFromEntity<T>(m_id);
				update_category_and_index();
			}

			template<typename T> void add(T&& t) {
				static_assert(allowEditing && sizeof(T) >= 0, "You took this entity from an ecs::view. Adding components like this is not allowed. Use edit_view to add components instead.");
				categorySource->editor().template attachToEntity<T>(m_id, std::forward<T>(t));
				update_category_and_index();
			}

			template<typename T> void add(T&& t, rynx::type_index::virtual_type type) {
				static_assert(allowEditing && sizeof(T) >= 0, "You took this entity from an ecs::view. Adding components like this is not allowed. Use edit_view to add components instead.");
				categorySource->editor().template type_alias<T>(type).template attachToEntity<T>(m_id, std::forward<T>(t));
				update_category_and_index();
			}

			entity_id_t id() const { return m_id; }
		private:
			category_source* categorySource;
			entity_category* m_entity_category;
			index_t m_category_index;
			entity_id_t m_id;
		};

		template<typename category_source>
		class const_entity {
		public:
			const_entity(const category_source& parent, entity_id_t id) : categorySource(&parent), m_id(id) {
				auto categoryAndIndex = categorySource->category_and_index_for(m_id);
				m_entity_category = categoryAndIndex.first;
				m_category_index = categoryAndIndex.second;
				rynx_assert(m_entity_category->ids().size() > m_category_index && m_entity_category->ids()[m_category_index] == id, "entity mapping is broken");
			}

			std::vector<rynx::reflection::type> reflections(rynx::reflection::reflections& reflections) const {
				std::vector<rynx::reflection::type> result;
				m_entity_category->forEachTable([&result, &reflections](rynx::ecs_internal::itable* tbl) {
					result.emplace_back(tbl->reflection(reflections));
				});
				return result;
			}

			template<typename T> const T& get() const {
				categorySource->template componentTypesAllowed<T>();
				rynx_assert(m_entity_category != nullptr, "referenced entity seems to not exist.");
				return m_entity_category->template table<T>()[m_category_index];
			}
			
			template<typename T> const T& get(rynx::type_index::virtual_type type) const {
				categorySource->template componentTypesAllowed<T>();
				rynx_assert(m_entity_category, "referenced entity seems to not exist.");
				return m_entity_category->template table<T>(type.type_value)[m_category_index];
			}

			template<typename T> const T* try_get() const {
				categorySource->template componentTypesAllowed<T>();
				type_id_t typeIndex = rynx::type_index::id<T>();
				rynx_assert(m_entity_category != nullptr, "referenced entity seems to not exist.");
				if (m_entity_category->types().test(typeIndex)) {
					return &m_entity_category->template table<T>(typeIndex)[m_category_index];
				}
				return nullptr;
			}
			template<typename... Ts> bool has() const {
				categorySource->template componentTypesAllowed<Ts...>();
				rynx_assert(m_entity_category != nullptr, "referenced entity seems to not exist.");
				return true & (m_entity_category->types().test(rynx::type_index::id<Ts>()) & ...);
			}
			bool has(rynx::type_index::virtual_type t) const noexcept {
				rynx_assert(m_entity_category, "referenced entity seems to not exist.");
				return m_entity_category->types().test(t.type_value);
			}

			template<typename... Ts> void remove() { static_assert(sizeof...(Ts) == 1 && sizeof...(Ts) == 2, "can't remove components from const entity"); }
			template<typename T, typename... Args> void add(Args&& ... args) const { static_assert(sizeof(T) == 1 && sizeof(T) == 2, "can't add components to const entity"); }
		private:
			const category_source* categorySource;
			const entity_category* m_entity_category;
			index_t m_category_index;
			entity_id_t m_id;
		};

		template<DataAccess accessType, typename category_source>
		class query_t {
			dynamic_bitset inTypes;
			dynamic_bitset notInTypes;
			category_source m_ecs;
			rynx::unordered_map<type_id_t, type_id_t> m_typeAliases;
			bool m_consumed = false;

		public:
			query_t(category_source& ecs) : m_ecs(ecs) {}
			query_t(const category_source& ecs_) : m_ecs(const_cast<category_source&>(ecs_)) {}
			~query_t() {}
			
			template<typename...Ts> query_t& in() { (inTypes.set(rynx::type_index::id<std::add_const_t<Ts>>()), ...); return *this; }
			template<typename...Ts> query_t& notIn() { (notInTypes.set(rynx::type_index::id<std::add_const_t<Ts>>()), ...); return *this; }
			
			query_t& in(rynx::type_index::virtual_type t) { inTypes.set(t.type_value); return *this; }
			query_t& notIn(rynx::type_index::virtual_type t) { notInTypes.set(t.type_value); return *this; }

			template<typename T> query_t& type_alias(type_index::virtual_type mapped_type) {
				m_typeAliases.emplace(rynx::type_index::id<T>(), mapped_type.type_value);
				return *this;
			}

			template<typename F>
			query_t& for_each(F&& op) {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				entity_iterator<accessType, category_source, decltype(&F::operator())> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(std::move(m_typeAliases));
				it.for_each(std::forward<F>(op));
				return *this;
			}

			template<typename F>
			rynx::ecs::range for_each_partial(rynx::ecs::range r, F&& op) {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				entity_iterator<accessType, category_source, decltype(&F::operator())> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(std::move(m_typeAliases));
				it.for_each_partial(r, std::forward<F>(op));
				index_t num = it.count();
				
				r.begin += r.step;
				if (r.begin >= num) {
					r.begin = 0;
				}

				return r;
			}

			template<typename F, typename TaskContext>
			auto for_each_parallel(TaskContext&& task_context, F&& op) {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				
				entity_iterator<accessType, category_source, decltype(&F::operator())> it(this->m_ecs);
				it.include(std::move(this->inTypes));
				it.exclude(std::move(this->notInTypes));
				it.type_aliases(std::move(this->m_typeAliases));
				return it.for_each_parallel(task_context, std::forward<F>(op));
			}

			template<typename F>
			query_t& for_each_buffer(F&& op) {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				buffer_iterator<accessType, category_source, decltype(&F::operator())> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(std::move(m_typeAliases));
				it.for_each_buffer(std::forward<F>(op));
				return *this;
			}

			std::vector<rynx::ecs::id> ids() {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				std::vector<rynx::ecs::id> result;
				gatherer<category_source> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(std::move(m_typeAliases));
				it.ids(result);
				return result;
			}

			template<typename F>
			std::vector<rynx::ecs::id> ids_if(F&& f) {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				entity_iterator<accessType, category_source, decltype(&F::operator())> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(std::move(m_typeAliases));
				return it.ids_if(std::forward<F>(f));
			}
			
			template<typename...Args>
			std::vector<std::tuple<Args...>> gather() {
				std::vector<std::tuple<Args...>> result;
				for_each([&result](Args... args) { result.emplace_back(args...); });
				return result;
			}

			index_t count() {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				gatherer<category_source> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(std::move(m_typeAliases));
				return it.count();
			}

			template<typename T>
			void sort_by() {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				sorter<category_source> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(std::move(m_typeAliases));
				it.template sort_categories_by<T>();
			}

			template<typename T>
			void sort_single_step_by() {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				sorter<category_source> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(std::move(m_typeAliases));
				it.template sort_buckets_one_step<T>();
			}
		};

		ecs() {
			rynx::type_index::initialize();
		}
		
		~ecs() {}

		constexpr size_t size() const {
			return m_idCategoryMap.size();
		}

		void serialize(rynx::reflection::reflections& reflections, rynx::serialization::vector_writer& out) {
			// first construct mapping from ecs ids to serialized ids.
			rynx::unordered_map<uint64_t, uint64_t> idToSerializedId;
			rynx::unordered_map<uint64_t, uint64_t> serializedIdToId;
			uint64_t next = 1;
			for (auto&& id : m_idCategoryMap) {
				idToSerializedId[id.first] = next;
				serializedIdToId[next] = id.first;
				++next;
			}

			// replace id fields with serialized id values.
			for (auto&& category : m_categories) {
				category.second->forEachTable([&idToSerializedId](rynx::ecs_internal::itable* table) {
					table->for_each_id_field([&idToSerializedId](rynx::id& id) { id.value = idToSerializedId.find(id.value)->second; });
				});
			}

			// serialize everything as-is.
			rynx::serialize(reflections, out); // include reflection of written data
			rynx::serialize(m_idCategoryMap.size(), out);
			
			size_t numNonEmptyCategories = 0;
			for (auto&& category : m_categories) {
				if (!category.second->ids().empty())
					++numNonEmptyCategories;
			}
			rynx::serialize(numNonEmptyCategories, out);
			// rynx::serialize(m_categories.size(), out);
			for (auto&& category : m_categories) {
				if (category.second->ids().empty())
					continue;

				std::vector<rynx::string> category_typenames;
				category.first.forEachOne([&category, &reflections, &category_typenames](uint64_t type_id) {
					auto* typeReflection = reflections.find(type_id);
					auto* tablePtr = category.second->table_ptr(type_id);
					if (tablePtr) {
						logmsg("%s", tablePtr->type_name().c_str());
					}
					else {
						if (typeReflection) {
							logmsg("no table for type: %s", typeReflection->m_type_name.c_str());
						}
						else {
							logmsg("no table for type %lld", type_id);
						}
					}
					// rynx_assert(typeReflection != nullptr, "serializing type that does not have reflection");
					if(typeReflection && typeReflection->m_serialization_allowed)
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


			// restore serialized id values back to original run-time id values.
			for (auto&& category : m_categories) {
				category.second->forEachTable([&serializedIdToId](rynx::ecs_internal::itable* table) {
					table->for_each_id_field([&serializedIdToId](rynx::id& id) { id.value = serializedIdToId.find(id.value)->second; });
				});
			}
		}

		rynx::serialization::vector_writer serialize(rynx::reflection::reflections& reflections) {
			rynx::serialization::vector_writer out;
			serialize(reflections, out);
			return out;
		}

		// serializes the top-most scene to memory.
		void serialize_scene(rynx::reflection::reflections& reflections, rynx::serialization::vector_writer& out) {
			// make a copy of ecs
			rynx::ecs copy; // = *this;
			
			// transform id references to global id chains. (???)
			
			// for the copy, remove all sub-scene content
			auto res = copy.query().gather<rynx::components::ecs::scene::children>();
			for (auto [children_component] : res) {
				for (auto id : children_component.entities) {
					copy.erase(id);
				}
			}
			
			// serialize as is.
			copy.serialize(reflections, out);
		}
		
		entity_range_t deserialize(rynx::reflection::reflections& reflections, rynx::serialization::vector_reader& in) {
			
			size_t numEntities;
			size_t numCategories;

			auto id_range_begin = m_entities.peek_next_id();

			rynx::reflection::reflections format;
			rynx::deserialize(format, in); // reflection of loaded data
			rynx::deserialize(numEntities, in);
			rynx::deserialize(numCategories, in);

			// construct mapping from serialized ids to ecs ids.
			rynx::unordered_map<uint64_t, uint64_t> serializedIdToEcsId;
			for (size_t i = 1; i <= numEntities; ++i) {
				serializedIdToEcsId[i] = this->m_entities.generateOne();
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
				if (m_categories.find(category_id) == m_categories.end()) {
					auto res = m_categories.emplace(category_id, rynx::make_unique<entity_category>(category_id));
					category_id.forEachOne([&](uint64_t typeId) {
						auto* typeReflection = reflections.find(typeId);
						res.first->second->createNewTable(typeId, typeReflection->m_create_table_func());
					});
				}
				auto category_it = m_categories.find(category_id);

				// source category typenames is the correct order to deserialize in.
				std::vector<type_id_t> additional_types;
				for (auto&& name : category_typenames) {
					auto reflection_ptr = reflections.find(name);
					auto type_index_value = reflection_ptr->m_type_index_value;
					auto* table_ptr = category_it->second->table(type_index_value, reflection_ptr->m_create_table_func);
					if (table_ptr != nullptr) {
						logmsg("deserializing table %s", name.c_str());
						table_ptr->deserialize(in);
						if (table_ptr->is_type_segregated()) {
							auto& segregation_map = value_segregated_types_map(type_index_value, reflection_ptr->m_create_map_func);
							void* segregated_data_ptr = table_ptr->get(0);
							if (!segregation_map.contains(segregated_data_ptr)) {
								segregation_map.emplace(segregated_data_ptr, create_virtual_type());
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
					id.value = serializedIdToEcsId[id.value];
				}

				auto idCount = category_it->second->m_ids.size();
				category_it->second->m_ids.insert(category_it->second->m_ids.end(), categoryIds.begin(), categoryIds.end());
				
				// restore correct runtime ids to deserialized entities id fields.
				category_it->second->forEachTable([idCount, &serializedIdToEcsId](rynx::ecs_internal::itable* table) {
					table->for_each_id_field_from_index(idCount, [&serializedIdToEcsId](rynx::id& id) {
						id.value = serializedIdToEcsId.find(id.value)->second;
					});
				});

				for (auto& id : categoryIds) {
					m_idCategoryMap[id.value] = { category_it->second.get(), index_t(idCount++) };
				}

				// add missing segregation types.
				if (!additional_types.empty()) {
					for (auto&& segregation_virtual_type : additional_types) {
						category_id.set(segregation_virtual_type);
					}

					// if category already does not exist - create category.
					if (m_categories.find(category_id) == m_categories.end()) {
						// if dst category does not previously exist, we can just move the current category in it's place.
						rynx::dynamic_bitset prev_category_id = std::move(category_it->second->m_types);
						category_it->second->m_types = category_id;
						auto res = m_categories.emplace(category_id, std::move(category_it->second));
						m_categories.erase(prev_category_id);
					}
					else {
						// TODO: Increase perf by migrating all entities from category at once.
						auto dst_category_it = m_categories.find(category_id);
						while (!category_it->second->ids().empty()) {
							dst_category_it->second->migrateEntityFrom(
								category_id,
								category_it->second.get(),
								0,
								this->m_idCategoryMap
							);
						}
						m_categories.erase(category_it);
					}
				}
			}

			return { id_range_begin, id_range_begin + numEntities };
		}

		bool exists(entity_id_t id) const { return m_idCategoryMap.find(id) != m_idCategoryMap.end(); }
		bool exists(id id) const { return exists(id.value); }

		entity<ecs, true> operator[](entity_id_t id) { return entity<ecs, true>(*this, id); }
		entity<ecs, true> operator[](id id) { return entity<ecs, true>(*this, id.value); }
		const_entity<ecs> operator[](entity_id_t id) const { return const_entity<ecs>(*this, id); }
		const_entity<ecs> operator[](id id) const { return const_entity<ecs>(*this, id.value); }

		class ecs_reference {
		public:
			ecs_reference(const ecs* instance) : m_ecs(const_cast<ecs*>(instance)) {}
			auto& categories() { return m_ecs->categories(); }
			const auto& categories() const { return m_ecs->categories(); }

			template<typename T> type_id_t typeId() { return rynx::type_index::id<T>(); }
			template<typename...Ts> void componentTypesAllowed() const { m_ecs->componentTypesAllowed<Ts...>(); };

			auto& entity_category_map() { return m_ecs->m_idCategoryMap; }

		private:
			ecs* m_ecs;
		};

		query_t<DataAccess::Mutable, ecs_reference> query() { return query_t<DataAccess::Mutable, ecs_reference>(ecs_reference(this)); }
		query_t<DataAccess::Const, ecs_reference> query() const { return query_t<DataAccess::Const, ecs_reference>(ecs_reference(this)); }

		/*
		// todo: offering these short-hands is probably detrimental to clarity.
		template<typename...Ts> std::vector<rynx::ecs::id> ids() const { return query().in<Ts...>().ids(); }
		template<typename...Ts> index_t count() const { return query().in<Ts...>().count(); }
		*/

		std::pair<entity_category*, index_t> category_and_index_for(entity_id_t id) const {
			auto it = m_idCategoryMap.find(id);
			rynx_assert(it != m_idCategoryMap.end(), "requesting category and index for an entity id that does not exist");
			return it->second;
		}

		void erase(entity_id_t entityId) {
			auto it = m_idCategoryMap.find(entityId);
			if (it != m_idCategoryMap.end()) [[likely]] {
				auto* category = it->second.first;
				index_t entity_index = it->second.second;
				
				category->erase(entity_index, m_idCategoryMap);
				erase_category_if_empty(category);
			}
		}
		
		template<typename T>
		void erase(const T& iterable) {
			for (auto&& id : iterable) {
				erase(id);
			}
		}

		void erase(id entityId) { erase(entityId.value); }

		void clear() {
			for (auto&& entry : m_value_segregated_types_maps) {
				for (auto&& virtual_type_value : entry.second->get_virtual_types()) {
					this->m_virtual_types_released.emplace_back(virtual_type_value.type_value);
				}
			}
			this->m_categories.clear();
			this->m_idCategoryMap.clear();
			this->m_value_segregated_types_maps.clear();
			this->m_entities.clear();
		}

		// ecs allow check. everything is allowed except editing values of value segregated types.
		// so value-segregated + mutable == error
		template<typename...Args> void componentTypesAllowed() const {
			static_assert(
				true &&
				(
					(
						!(std::is_base_of_v<rynx::ecs_value_segregated_component_tag, std::remove_cvref_t<Args>> &&
						(!std::is_const_v<std::remove_reference_t<Args>> && std::is_reference_v<Args>))
					)
					&& ...
				)
				, "you cannot edit value segregated types' values. erase the component and add again with different value."
			);
		}

		template<typename T> type_id_t type_id_for([[maybe_unused]] const T& t) {
			if constexpr (std::is_same_v<std::remove_cvref_t<T>, rynx::type_index::virtual_type>) {
				return t.type_value;
			}
			else {
				return rynx::type_index::id<T>();
			}
		}

		void erase_category_if_empty(entity_category* source_category) {
			return;
			if (source_category->ids().empty()) {
				rynx_assert(m_categories.find(source_category->types())->second.get() == source_category, "???");
				m_categories.erase(source_category->types());
			}
		}

		class edit_t {
		private:
			ecs& m_ecs;
			rynx::unordered_map<type_id_t, type_id_t> m_typeAliases;

			type_id_t mapped_type_id(type_id_t v) const {
				auto it = m_typeAliases.find(v);
				if (it != m_typeAliases.end())
					return it->second;
				return v;
			}

			template<typename T>
			void compute_type_category(rynx::dynamic_bitset& dst, T& component) {
				dst.set(mapped_type_id(m_ecs.type_id_for(component)));
				if constexpr (std::is_base_of_v<rynx::ecs_value_segregated_component_tag, std::remove_cvref_t<T>>) {
					auto& map = m_ecs.value_segregated_types_map<std::remove_cvref_t<T>>();
					auto it = map.find(component);
					if (it == map.end()) {
						auto v_type = m_ecs.create_virtual_type();
						map.emplace(component, v_type);
						dst.set(v_type.type_value);
					}
					else {
						dst.set(it->second.type_value);
					}
				}
			}

			void compute_type_category(
				rynx::dynamic_bitset& dst,
				type_id_t type_id,
				bool is_value_segregated,
				rynx::function<rynx::unique_ptr<rynx::ecs_internal::ivalue_segregation_map>()> map_create_func,
				void* data)
			{
				dst.set(mapped_type_id(type_id));
				if (is_value_segregated) {
					auto& map = m_ecs.value_segregated_types_map(type_id, map_create_func);
					if (!map.contains(data)) {
						auto v_type = m_ecs.create_virtual_type();
						map.emplace(data, v_type);
						dst.set(v_type.type_value);
					}
					else {
						dst.set(map.get_type_id_for(data).type_value);
					}
				}
			}

			template<typename T>
			void compute_type_category_n(rynx::dynamic_bitset& dst, std::vector<T>& component) {
				dst.set(mapped_type_id(m_ecs.type_id_for(component.front())));
				if constexpr (std::is_base_of_v<rynx::ecs_value_segregated_component_tag, std::remove_cvref_t<T>>) {
					auto& map = m_ecs.value_segregated_types_map<std::remove_cvref_t<T>>();
					auto it = map.find(component.front());
					if (it == map.end()) {
						auto v_type = m_ecs.create_virtual_type();
						map.emplace(component.front(), v_type);
						dst.set(v_type.type_value);
					}
					else {
						dst.set(it->second.type_value);
					}
				}
			}

			template<typename T>
			void reset_component(rynx::dynamic_bitset& dst, [[maybe_unused]] entity_id_t id) {
				dst.reset(mapped_type_id(rynx::type_index::id<T>()));
				if constexpr (std::is_base_of_v<rynx::ecs_value_segregated_component_tag, std::remove_cvref_t<T>>) {
					auto& map = m_ecs.value_segregated_types_map<std::remove_cvref_t<T>>();

					auto it = map.find(m_ecs[id].get<const T>());
					rynx_assert(it != map.end(), "entity has component T with some value. corresponding virtual type must exist.");
					dst.reset(it->second.type_value);
				}
			}

		public:
			edit_t(ecs& host) : m_ecs(host) {}

			template<typename T> edit_t& type_alias(type_index::virtual_type mapped_type) {
				m_typeAliases.emplace(rynx::type_index::id<T>(), mapped_type.type_value);
				return *this;
			}

			template<typename... Components>
			entity_id_t create(Components&& ... components) {
				entity_id_t id = m_ecs.m_entities.generateOne();
				dynamic_bitset targetCategory;
				(compute_type_category(targetCategory, components), ...);
				auto category_it = m_ecs.m_categories.find(targetCategory);
				if (category_it == m_ecs.m_categories.end()) { category_it = m_ecs.m_categories.emplace(targetCategory, rynx::make_unique<entity_category>(targetCategory)).first; }
				auto index = category_it->second->insertNew(m_typeAliases, id, std::forward<Components>(components)...);
				m_ecs.m_idCategoryMap.emplace(id, std::make_pair(category_it->second.get(), index_t(index)));
				return id;
			}

			// TODO: The api for tags is super unclear for user. Should have some kind of ..
			//       create_n(ids, vector<Ts>).with_tags<Tag1, Tag2...>(); // style format.
			template<typename... Tags, typename... Components>
			std::vector<entity_id_t> create_n(std::vector<Components>& ... components) {
				rynx_assert((components.size() & ...) == (components.size() | ...), "components vector sizes do not match!");
				
				if constexpr (true) {
					std::vector<entity_id_t> ids(rynx::first_of(components...).size());

					// TODO: generate_n might be more efficient. benchmark if this shows up in profiler.
					for (entity_id_t& id : ids) {
						id = m_ecs.m_entities.generateOne();
					}
					rynx_assert(rynx::first_of(components...).size() == ids.size(), "ahaa");

					// target category is the same for all entities created in this call.
					dynamic_bitset targetCategory;
					(compute_type_category_n(targetCategory, components), ...);
					(targetCategory.set(mapped_type_id(rynx::type_index::id<Tags>())), ...);
					auto category_it = m_ecs.m_categories.find(targetCategory);
					if (category_it == m_ecs.m_categories.end()) { category_it = m_ecs.m_categories.emplace(targetCategory, rynx::make_unique<entity_category>(targetCategory)).first; }

					auto first_index = category_it->second->insertNew(m_typeAliases, ids, components...);
					for (size_t i = 0; i < ids.size(); ++i) {
						m_ecs.m_idCategoryMap.emplace(ids[i], std::make_pair(category_it->second.get(), index_t(first_index + i)));
					}
					return ids;
				}
				else {
					std::vector<entity_id_t> created_ids;
					auto num_entities = rynx::first_of(components...).size();
					for (size_t i = 0; i < num_entities; ++i) {
						created_ids.emplace_back(create(std::move(components[i])...));
					}
					return created_ids;
				}
			}

			edit_t& attachToEntity_typeErased(
				entity_id_t id,
				type_id_t type_id,
				bool is_value_segregated,
				rynx::ecs_table_create_func& table_create_func,
				rynx::function<rynx::unique_ptr<rynx::ecs_internal::ivalue_segregation_map>()> map_create_func,
				opaque_unique_ptr<void> component)
			{
				auto it = m_ecs.m_idCategoryMap.find(id);
				auto* source_category = it->second.first;
				index_t source_index = it->second.second;

				rynx_assert(it != m_ecs.m_idCategoryMap.end(), "attachToEntity called for entity that does not exist.");
				rynx_assert(source_category->ids()[source_index] == id, "Entity mapping is broken");

				const dynamic_bitset& initialTypes = source_category->types();
				dynamic_bitset resultTypes = initialTypes;
				compute_type_category(resultTypes, type_id, is_value_segregated, map_create_func, component.get());

				auto destinationCategoryIt = m_ecs.m_categories.find(resultTypes);
				if (destinationCategoryIt == m_ecs.m_categories.end()) {
					destinationCategoryIt = m_ecs.m_categories.emplace(resultTypes, rynx::make_unique<entity_category>(resultTypes)).first;
					destinationCategoryIt->second->copyTypesFrom(source_category); // NOTE: Must make copies of table types for tables for which we don't know the type in this context.
				}

				rynx_assert(destinationCategoryIt->second->types() != source_category->types(), "attaching component to entity, but entity already has that component!");

				// first migrate the old stuff, then apply on top of that what was given (in case these types overlap).
				destinationCategoryIt->second->migrateEntityFrom(
					initialTypes, // types moved
					source_category, // source category
					source_index, // source index
					m_ecs.m_idCategoryMap
				);

				m_ecs.erase_category_if_empty(source_category);
				destinationCategoryIt->second->insertComponent_typeErased(m_typeAliases, type_id, table_create_func, std::move(component));
				return *this;
			}

			template<typename... Components>
			edit_t& attachToEntity(entity_id_t id, Components&& ... components) {
				auto it = m_ecs.m_idCategoryMap.find(id);
				auto* source_category = it->second.first;
				index_t source_index = it->second.second;

				rynx_assert(it != m_ecs.m_idCategoryMap.end(), "attachToEntity called for entity that does not exist.");
				rynx_assert(source_category->ids()[source_index] == id, "Entity mapping is broken");
				
				const dynamic_bitset& initialTypes = source_category->types();
				dynamic_bitset resultTypes = initialTypes;
				(compute_type_category(resultTypes, components), ...);
				
				auto destinationCategoryIt = m_ecs.m_categories.find(resultTypes);
				if (destinationCategoryIt == m_ecs.m_categories.end()) {
					destinationCategoryIt = m_ecs.m_categories.emplace(resultTypes, rynx::make_unique<entity_category>(resultTypes)).first;
					destinationCategoryIt->second->copyTypesFrom(source_category); // NOTE: Must make copies of table types for tables for which we don't know the type in this context.
				}

				rynx_assert(destinationCategoryIt->second->types() != source_category->types(), "attaching component to entity, but entity already has that component!");

				// first migrate the old stuff, then apply on top of that what was given (in case these types overlap).
				destinationCategoryIt->second->migrateEntityFrom(
					initialTypes, // types moved
					source_category, // source category
					source_index, // source index
					m_ecs.m_idCategoryMap
				);
				
				m_ecs.erase_category_if_empty(source_category);
				destinationCategoryIt->second->insertComponents(m_typeAliases, std::forward<Components>(components)...);
				return *this;
			}

			template<typename... Components>
			edit_t& removeFromEntity(entity_id_t id) {
				auto it = m_ecs.m_idCategoryMap.find(id);
				auto* source_category = it->second.first;
				index_t source_index = it->second.second;

				rynx_assert(it != m_ecs.m_idCategoryMap.end(), "removeFromEntity called for entity that does not exist.");
				rynx_assert(source_category->ids()[source_index] == id, "Entity mapping is broken");

				const dynamic_bitset& initialTypes = source_category->types();
				dynamic_bitset resultTypes = initialTypes;

				(reset_component<Components>(resultTypes, id), ...);

				auto destinationCategoryIt = m_ecs.m_categories.find(resultTypes);
				if (destinationCategoryIt == m_ecs.m_categories.end()) {
					destinationCategoryIt = m_ecs.m_categories.emplace(resultTypes, rynx::make_unique<entity_category>(resultTypes)).first;
					destinationCategoryIt->second->copyTypesFrom(source_category, resultTypes);
				}

				source_category->eraseComponentsFromIndex<Components...>(m_typeAliases, source_index);
				destinationCategoryIt->second->migrateEntityFrom(
					resultTypes,
					source_category,
					source_index,
					m_ecs.m_idCategoryMap
				);
				
				m_ecs.erase_category_if_empty(source_category);
				return *this;
			}

			edit_t& removeFromEntity(entity_id_t id, type_id_t t) {
				auto it = m_ecs.m_idCategoryMap.find(id);
				rynx_assert(it != m_ecs.m_idCategoryMap.end(), "removeFromEntity called for entity that does not exist.");

				auto* source_category = it->second.first;
				index_t source_index = it->second.second;

				const dynamic_bitset& initialTypes = source_category->types();
				dynamic_bitset resultTypes = initialTypes;
				resultTypes.reset(t);

				auto destinationCategoryIt = m_ecs.m_categories.find(resultTypes);
				if (destinationCategoryIt == m_ecs.m_categories.end()) {
					destinationCategoryIt = m_ecs.m_categories.emplace(resultTypes, rynx::make_unique<entity_category>(resultTypes)).first;
					destinationCategoryIt->second->copyTypesFrom(source_category, resultTypes);
				}

				if (m_ecs.m_value_segregated_types_maps.find(t) == m_ecs.m_value_segregated_types_maps.end()) {
					source_category->eraseComponentFromIndex(m_typeAliases, source_index, t);
				}
				destinationCategoryIt->second->migrateEntityFrom(
					resultTypes,
					source_category,
					source_index,
					m_ecs.m_idCategoryMap
				);

				m_ecs.erase_category_if_empty(source_category);
				return *this;
			}

			edit_t& removeFromEntity(rynx::id id, type_id_t t) {
				return removeFromEntity(id.value, t);
			}

			edit_t& removeFromEntity(entity_id_t id, rynx::type_index::virtual_type t) {
				auto it = m_ecs.m_idCategoryMap.find(id);
				rynx_assert(it != m_ecs.m_idCategoryMap.end(), "removeFromEntity called for entity that does not exist.");
				
				auto* source_category = it->second.first;
				index_t source_index = it->second.second;

				const dynamic_bitset& initialTypes = source_category->types();
				dynamic_bitset resultTypes = initialTypes;
				resultTypes.reset(t.type_value);

				auto destinationCategoryIt = m_ecs.m_categories.find(resultTypes);
				if (destinationCategoryIt == m_ecs.m_categories.end()) {
					destinationCategoryIt = m_ecs.m_categories.emplace(resultTypes, rynx::make_unique<entity_category>(resultTypes)).first;
					destinationCategoryIt->second->copyTypesFrom(source_category, resultTypes);
				}

				// normally we would need to erase the components from source category entity first.
				// but no need to erase any actual data from actual vectors here, since virtual types do not describe actual data types.
				destinationCategoryIt->second->migrateEntityFrom(
					resultTypes,
					source_category,
					source_index,
					m_ecs.m_idCategoryMap
				);
				
				m_ecs.erase_category_if_empty(source_category);
				return *this;
			}
		};

		edit_t editor() {
			return edit_t(*this);
		}

		template<typename... Components>
		entity_id_t create(Components&& ... components) {
			return edit_t(*this).create(std::forward<Components>(components)...);
		}

		template<typename... Tags, typename... Components>
		std::vector<entity_id_t> create_n(std::vector<Components>& ... components) {
			return edit_t(*this).create_n<Tags..., Components...>(components...);
		}

		template<typename... Components>
		edit_t& attachToEntity(entity_id_t id, Components&& ... components) {
			return edit_t(*this).attachToEntity(id, std::forward<Components>(components)...);
		}

		template<typename... Components>
		edit_t& removeFromEntity(entity_id_t id) {
			return edit_t(*this).removeFromEntity<Components...>(id);
		}

		edit_t& removeFromEntity(entity_id_t id, rynx::type_index::virtual_type t) {
			return edit_t(*this).removeFromEntity(id, t);
		}

		template<typename... Components> ecs& attachToEntity(rynx::id id, Components&& ... components) { attachToEntity(id.value, std::forward<Components>(components)...); return *this; }
		template<typename... Components> ecs& removeFromEntity(rynx::id id) { removeFromEntity<Components...>(id.value); return *this; }

		rynx::type_index::virtual_type create_virtual_type() {
			if (!m_virtual_types_released.empty()) {
				auto virtual_type = m_virtual_types_released.back();
				m_virtual_types_released.pop_back();
				return { virtual_type };
			}
			return rynx::type_index::create_virtual_type();
		}

	public:
		// ecs view that operates on Args types only. generates compile errors if trying to access any other data.
		// also does not allow creating new components, removing existing components or creating new entities.
		// if you need to do that, use edit_view instead.
		template<typename...TypeConstraints>
		class view {
			template<typename T, bool> friend class rynx::ecs::entity;
			template<typename T> friend class rynx::ecs::const_entity;

			template<typename category_source> friend class rynx::ecs::gatherer;
			template<DataAccess accessType, typename category_source> friend class rynx::ecs::query_t;
			template<DataAccess accessType, typename category_source, typename F> friend class rynx::ecs::entity_iterator;
			template<DataAccess accessType, typename category_source, typename F> friend class rynx::ecs::buffer_iterator;

			template<typename T> static constexpr bool typeAllowed() { return isAny<std::remove_const_t<T>, id, rynx::type_index::virtual_type, std::remove_const_t<TypeConstraints>...>(); }
			template<typename T> static constexpr bool typeConstCorrect() {
				if constexpr (std::is_reference_v<T>) {
					if constexpr (std::is_const_v<std::remove_reference_t<T>>)
						return isAny<std::remove_reference_t<T>, id, rynx::type_index::virtual_type, std::add_const_t<TypeConstraints>...>(); // user must have requested any access to type (read/write both acceptable).
					else
						return isAny<std::remove_reference_t<T>, id, rynx::type_index::virtual_type, TypeConstraints...>(); // verify user has requested a write access.
				}
				else {
					// user is taking a copy, so it is always const with regards to what we have stored.
					return isAny<std::remove_const_t<T>, id, rynx::type_index::virtual_type, std::remove_const_t<TypeConstraints>...>();
				}
			}

			template<typename...Args> static constexpr bool typesAllowed() { return true && (typeAllowed<std::remove_reference_t<Args>>() && ...); }
			template<typename...Args> static constexpr bool typesConstCorrect() { return true && (typeConstCorrect<Args>() && ...); }
			template<typename Arg> static constexpr bool valueSegregatedTypeAccessedOnlyInConstContext() {
				return 	!(std::is_base_of_v<rynx::ecs_value_segregated_component_tag, std::remove_cvref_t<Arg>> &&
					(!std::is_const_v<std::remove_reference_t<Arg>> && std::is_reference_v<Arg>));
			}
			auto& entity_category_map() { return m_ecs->m_idCategoryMap; }

		protected:
			template<typename...Args> void componentTypesAllowedForRemove() const {
				static_assert(typesAllowed<Args...>(), "Your ecs view does not have access to one of requested types.");
				static_assert(typesConstCorrect<Args...>(), "You promised to access type in only const context.");
			}
			
			template<typename...Args> void componentTypesAllowed() const {
				static_assert(typesAllowed<Args...>(), "Your ecs view does not have access to one of requested types.");
				static_assert(typesConstCorrect<Args...>(), "You promised to access type in only const context.");
				static_assert(true && (valueSegregatedTypeAccessedOnlyInConstContext<Args>() && ...), "");
			}

		public:
			view(const rynx::ecs* ecs) : m_ecs(const_cast<rynx::ecs*>(ecs)) {}

			bool exists(entity_id_t id) const { return m_ecs->exists(id); }
			bool exists(id id) const { return exists(id.value); }

			entity<view, false> operator[](entity_id_t id) { return entity<view, false>(*this, id); }
			entity<view, false> operator[](id id) { return entity<view, false>(*this, id.value); }

			const_entity<view> operator[](entity_id_t id) const { return const_entity<view>(*this, id); }
			const_entity<view> operator[](id id) const { return const_entity<view>(*this, id.value); }

			query_t<DataAccess::Mutable, view> query() { return query_t<DataAccess::Mutable, view>(*this); }
			query_t<DataAccess::Const, view> query() const { return query_t<DataAccess::Const, view>(*this); }

			rynx::type_index::virtual_type create_virtual_type() { return m_ecs->create_virtual_type(); }

		private:
			auto& categories() { return m_ecs->categories(); }
			const auto& categories() const { return m_ecs->categories(); }

			auto category_and_index_for(entity_id_t id) const { return m_ecs->category_and_index_for(id); }

		protected:
			template<typename T>
			static constexpr bool type_verify() {
				if constexpr (std::is_const_v<T>) {
					return isAny<T, std::add_const_t<TypeConstraints>...>();
				}
				else {
					return isAny<T, TypeConstraints...>();
				}
			}

		public:
			template<typename...Args> operator view<Args...>() const {
				static_assert((type_verify<Args>() && ...), "all requested types must be present in parent view");
				return view<Args...>(m_ecs);
			}

		protected:
			rynx::ecs* m_ecs;
		};

		template<typename...TypeConstraints>
		class edit_view final : public view<TypeConstraints...> {
		public:
			edit_view(const rynx::ecs* ecs) : view<TypeConstraints...>(const_cast<rynx::ecs*>(ecs)) {}
			
			entity<edit_view, true> operator[](entity_id_t id) { return entity<edit_view, true>(*this, id); }
			entity<edit_view, true> operator[](id id) { return entity<edit_view, true>(*this, id.value); }

			auto operator[](entity_id_t id) const { return const_entity<view<TypeConstraints...>>(*this, id); }
			auto operator[](id id) const { return const_entity<view<TypeConstraints...>>(*this, id.value); }

			edit_t editor() {
				return edit_t(*this->m_ecs);
			}

			template<typename... Components>
			entity_id_t create(Components&& ... components) {
				this->template componentTypesAllowed<Components...>();
				return this->m_ecs->create(std::forward<Components>(components)...);
			}

			template<typename... Components>
			std::vector<entity_id_t> create_n(std::vector<Components>& ... components) {
				this->template componentTypesAllowed<Components...>();
				return this->m_ecs->create(components...);
			}

			template<typename... Components>
			edit_view& attachToEntity(entity_id_t id, Components&& ... components) {
				this->template componentTypesAllowed<Components...>();
				this->m_ecs->attachToEntity(id, std::forward<Components>(components)...);
				return *this;
			}
			template<typename... Components> edit_view& attachToEntity(id id, Components&& ... components) { return attachToEntity(id.value, std::forward<Components>(components)...); }

			template<typename... Components>
			edit_view& removeFromEntity(entity_id_t id) {
				this->template componentTypesAllowedForRemove<Components...>();
				this->m_ecs->template removeFromEntity<Components...>(id);
				return *this;
			}
			template<typename... Components> edit_view& removeFromEntity(id id) { return removeFromEntity<Components...>(id.value); }

			template<typename...Args> operator edit_view<Args...>() {
				static_assert((typename view<TypeConstraints...>::template type_verify<Args>() && ...), "all requested types must be present in parent view");
				return edit_view<Args...>(this->m_ecs);
			}
		};

		template<typename...Args> operator view<Args...>() const { return view<Args...>(this); }
		template<typename...Args> operator edit_view<Args...>() const { return edit_view<Args...>(this); }
	};

}
