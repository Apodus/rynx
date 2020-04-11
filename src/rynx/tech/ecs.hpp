#pragma once

#include <rynx/tech/dynamic_bitset.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/type_index.hpp>

#include <rynx/tech/profiling.hpp>

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
		using type_id_t = uint64_t;
		using entity_id_t = uint64_t;
		using index_t = uint32_t;

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

		// NOTE: Since we are using categories now, it really doesn't matter how far apart the entity ids are.
		//       Reusing ids is thus very much not important, unless we get to the point of overflow.
		//       But still it would be easier to "reset" entity_index between levels or something.
		class entity_index {
		public:
			entity_id_t generateOne() {
				return ++m_nextId;
			} // zero is never generated. we can use that as InvalidId.

			static constexpr entity_id_t InvalidId = 0;
		private:
			
			// does not need to be atomic. creating new entities is not thread-safe anyway, only one thread is allowed to do so at a time.
			entity_id_t m_nextId = 0;
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

		type_index m_types;
		entity_index m_entities;

		rynx::unordered_map<entity_id_t, std::pair<entity_category*, index_t>> m_idCategoryMap;
		rynx::unordered_map<dynamic_bitset, std::unique_ptr<entity_category>, bitset_hash> m_categories;

		template<typename T> type_id_t typeId() const { return m_types.template id<T>(); }

		auto& categories() { return m_categories; }
		const auto& categories() const { return m_categories; }

	public:
		struct id {
			id() : value(entity_index::InvalidId) {}
			id(entity_id_t v) : value(v) {}
			bool operator == (const id& other) const { return value == other.value; }

			// TODO: make value private, give const access only via member functions.
			entity_id_t value;
		};

		class itable {
		public:
			virtual ~itable() {}
			virtual void erase(entity_id_t entityId) = 0;
			
			virtual void swap_adjacent_indices_for(const std::vector<index_t>& index_points) = 0;
			virtual void swap_to_given_order(const std::vector<index_t>& relative_sort_order) = 0;

			// virtual itable* deepCopy() const = 0;
			virtual void copyTableTypeTo(type_id_t typeId, std::vector<std::unique_ptr<itable>>& targetTables) = 0;
			virtual void moveFromIndexTo(index_t index, itable* dst) = 0;
		};

		template<typename T>
		class component_table : public itable {
		public:
			virtual ~component_table() {}

			void insert(T&& t) { m_data.emplace_back(std::forward<T>(t)); }
			void insert(std::vector<T>& v) { m_data.insert(m_data.end(), v.begin(), v.end()); }
			template<typename...Ts> void emplace_back(Ts&& ... ts) { m_data.emplace_back(std::forward<Ts>(ts)...); }

			virtual void swap_adjacent_indices_for(const std::vector<index_t>& index_points) override {
				for (auto index : index_points) {
					std::swap(m_data[index], m_data[index + 1]);
				}
			}

			virtual void swap_to_given_order(const std::vector<index_t>& relative_sort_order) override {
				// TODO: No need to alloc these separately for each table. Reuse buffers and memcpy contents.
				std::vector<index_t> current_index_to_original_index(m_data.size());
				std::vector<index_t> original_index_to_current_index(m_data.size());

				std::iota(current_index_to_original_index.begin(), current_index_to_original_index.end(), 0);
				std::iota(original_index_to_current_index.begin(), original_index_to_current_index.end(), 0);

				for (index_t i = 0, size = index_t(m_data.size()); i < size; ++i) {
					index_t mapped_location = original_index_to_current_index[relative_sort_order[i]];
					if (mapped_location != i) {
						std::swap(m_data[i], m_data[mapped_location]);
						
						std::swap(current_index_to_original_index[i], current_index_to_original_index[mapped_location]);
						std::swap(
							original_index_to_current_index[current_index_to_original_index[i]],
							original_index_to_current_index[current_index_to_original_index[mapped_location]]
						);
					}
				}
			}

			virtual void erase(entity_id_t id) override {
				m_data[id] = std::move(m_data.back());
				m_data.pop_back();
			}

			void insert_default(size_t n) {
				m_data.resize(m_data.size() + n);
			}

			// NOTE: This is required for moving entities between categories reliably.
			virtual void copyTableTypeTo(type_id_t typeId, std::vector<std::unique_ptr<itable>>& targetTables) override {
				if (typeId >= targetTables.size()) {
					targetTables.resize(((3 * typeId) >> 1) + 1);
				}
				targetTables[typeId] = std::make_unique<component_table>();
			}

			virtual void moveFromIndexTo(index_t index, itable* dst) override {
				static_cast<component_table*>(dst)->emplace_back(std::move(m_data[index]));
				if(index + 1 != static_cast<index_t>(m_data.size()))
					m_data[index] = std::move(m_data.back());
				m_data.pop_back();
			}

			T& back() { return m_data.back(); }
			const T& back() const { return m_data.back(); }
			void pop_back() { m_data.pop_back(); }
			
			T* data() { return m_data.data(); }
			const T* data() const { return m_data.data(); }

			size_t size() const { return m_data.size(); }

			T& operator[](index_t index) { return m_data[index]; }
			const T& operator[](index_t index) const { return m_data[index]; }

		private:
			std::vector<std::remove_reference_t<T>> m_data;
		};

		class entity_category;
		struct migrate_move {
			migrate_move(entity_id_t id_, index_t newIndex_, entity_category* new_category_) : id(id_), newIndex(newIndex_), new_category(new_category_) {}
			entity_id_t id;
			index_t newIndex;
			entity_category* new_category;
		};

		class entity_category {
		public:
			entity_category(dynamic_bitset types, type_index* typeIndex) : m_types(std::move(types)), m_typeIndex(typeIndex) {}

			bool includesAll(const dynamic_bitset& types) const { return m_types.includes(types); }
			bool includesNone(const dynamic_bitset& types) const { return m_types.excludes(types); }

			void erase(index_t index, rynx::unordered_map<entity_id_t, std::pair<entity_category*, index_t>>& idmap) {
				for (auto&& table : m_tables) {
					if(table)
						table->erase(index);
				}

				rynx_assert(index < m_ids.size(), "out of bounds");
				auto erasedEntityId = m_ids[index];
				m_ids[index] = std::move(m_ids.back());
				m_ids.pop_back();
				
				// update ecs-wide id<->category mapping
				idmap.erase(erasedEntityId.value);
				if (index != m_ids.size()) {
					idmap[m_ids[index].value] = { this, index };
				}
			}

			// TODO: Rename better. This is like bubble-sort single step.
			// TODO: Use some smarter algorithm?
			template<typename T> void sort_one_step(rynx::unordered_map<entity_id_t, std::pair<entity_category*, index_t>>& idmap) {
				auto type_index_of_t = m_typeIndex->id<T>();
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
				auto type_index_of_t = m_typeIndex->id<T>();
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
				if constexpr (!std::is_same_v<std::remove_cvref_t<T>, rynx::type_index::virtual_type>) {
					table<T>(typeAliases).emplace_back(std::forward<T>(component));
				}
			}

			template<typename T> void insertSingleComponentVector(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, std::vector<T>& component) {
				if constexpr (!std::is_same_v<std::remove_cvref_t<T>, rynx::type_index::virtual_type>) {
					table<T>(typeAliases).insert(component);
				}
			}

		public:
			template<typename... Tags, typename...Components> size_t insertNew(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, std::vector<entity_id_t>& ids, std::vector<Components>& ... components) {
				(insertSingleComponentVector(typeAliases, components), ...);
				size_t index = m_ids.size();
				m_ids.insert(m_ids.end(), ids.begin(), ids.end());

				auto insert_tags_table = [num = ids.size()](auto& table) { table.insert_default(num); };
				(insert_tags_table(table<Tags>()), ...);

				return index;
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

			template<typename Component> void eraseComponentFromIndex(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, index_t index) {
				auto& t = table<Component>(typeAliases);
				t[index] = std::move(t.back());
				t.pop_back();
			}

			template<typename...Components> void eraseComponentsFromIndex(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases, index_t index) { (eraseComponentFromIndex<Components>(typeAliases, index), ...); }

			void copyTypesFrom(entity_category* other, const dynamic_bitset& types) {
				type_id_t typeId = types.nextOne(0);
				while (typeId != dynamic_bitset::npos) {
					// TODO: Get rid of null check :( it is guarding against virtual types now.
					if (other->m_tables[typeId]) {
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

			void migrateEntity(
				const dynamic_bitset& types,
				entity_category* source,
				index_t source_index,
				rynx::unordered_map<entity_id_t, std::pair<entity_category*, index_t>>& idmap
			) {
				type_id_t typeId = types.nextOne(0);
				while (typeId != dynamic_bitset::npos) {
					// virtual types are present in bitset, but do not have a table.
					// TODO: mark virtual types in a separate bitset or something and compare against that. this nullcheck can hide actual bugs.
					if (m_tables[typeId]) {
						source->m_tables[typeId]->moveFromIndexTo(source_index, m_tables[typeId].get());
					}
					typeId = types.nextOne(typeId + 1);
				}

				m_ids.emplace_back(source->m_ids[source_index]);
				source->m_ids[source_index] = source->m_ids.back();
				source->m_ids.pop_back();

				// update mapping
				idmap.find(m_ids.back().value)->second = { this, index_t(m_ids.size() - 1) };
				if (source->m_ids.size() != source_index) {
					idmap.find(source->m_ids[source_index].value)->second = { source, index_t(source_index) };
				}
			}

			template<typename T, typename U = std::remove_const_t<std::remove_reference_t<T>>> component_table<U>& table(type_id_t typeIndex) {
				if (typeIndex >= m_tables.size()) {
					m_tables.resize(((3 * typeIndex) >> 1) + 1);
				}
				if (!m_tables[typeIndex]) {
					m_tables[typeIndex] = std::make_unique<component_table<U>>();
				}
				return *static_cast<component_table<U>*>(m_tables[typeIndex].get());
			}
			template<typename T, typename U = std::remove_const_t<std::remove_reference_t<T>>> component_table<U>& table() {
				type_id_t typeIndex = m_typeIndex->id<U>();
				return table<U>(typeIndex);
			}

			template<typename T> auto& table(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases) {
				type_id_t original = m_typeIndex->id<T>();
				auto it = typeAliases.find(original);
				if (it == typeAliases.end()) {
					return this->table<T>(original);
				}
				return this->table<T>(it->second);
			}

			template<typename T> const auto& table(type_id_t typeIndex) const { return const_cast<entity_category*>(this)->table<T>(typeIndex); }
			template<typename T> const auto& table() const { return const_cast<entity_category*>(this)->table<T>(); }
			template<typename T> const auto& table(const rynx::unordered_map<type_id_t, type_id_t>& typeAliases) const { return const_cast<entity_category*>(this)->table<T>(typeAliases); }

		private:
			template<DataAccess, typename Ts> struct getTables {};
			template<DataAccess accessType, typename... Ts> struct getTables<accessType, std::tuple<Ts...>> {
				auto operator()(entity_category& categorySource) {
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
				auto operator()(entity_category& categorySource, const rynx::unordered_map<type_id_t, type_id_t>& typeAliases) {
					if constexpr (accessType == DataAccess::Const) {
						return const_cast<const entity_category&>(categorySource).table<T>(typeAliases).data();
					}
					else {
						return categorySource.table<T>(typeAliases).data();
					}
				}
			};

			dynamic_bitset m_types;
			std::vector<std::unique_ptr<itable>> m_tables;
			std::vector<id> m_ids;
			type_index* m_typeIndex;
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

			gatherer& type_aliases(rynx::unordered_map<type_id_t, type_id_t>& typeAliases) {
				m_typeAliases = &typeAliases;
				return *this;
			}

			type_id_t use_mapped_type_if_present(type_id_t v) {
				auto it = m_typeAliases->find(v);
				if (it != m_typeAliases->end()) {
					return it->second;
				}
				return v;
			}
		protected:
			template<typename TupleType, size_t...Is> void unpack_types(std::index_sequence<Is...>) {
				(includeTypes.set(
					use_mapped_type_if_present(
						m_ecs.template typeId<typename std::tuple_element<Is, TupleType>::type>()
					)
				), ...);
			}
			template<typename... Ts> void unpack_types() {
				(includeTypes.set(
					use_mapped_type_if_present(
						m_ecs.template typeId<Ts>()
					)
				), ...);
			}

			template<typename TupleType, size_t...Is> void unpack_types_exclude(std::index_sequence<Is...>) {
				(excludeTypes.set(
					use_mapped_type_if_present(
						m_ecs.template typeId<typename std::tuple_element<Is, TupleType>::type>()
					)
				), ...);
			}
			
			template<typename... Ts> void unpack_types_exclude() {
				(excludeTypes.set(
					use_mapped_type_if_present(
						m_ecs.template typeId<Ts>()
					)
				), ...);
			}

			dynamic_bitset includeTypes;
			dynamic_bitset excludeTypes;
			rynx::unordered_map<type_id_t, type_id_t>* m_typeAliases = nullptr;
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
			buffer_iterator(category_source& ecs) : gatherer<category_source>(ecs) {
				this->m_ecs.template componentTypesAllowed<std::remove_pointer_t<Args>...>();
				static_assert(!std::is_same_v<FArg, rynx::ecs::id*>, "id pointer must be const when iterating buffers!");
				if constexpr (!std::is_same_v<FArg, rynx::ecs::id const*>) {
					this->m_ecs.template componentTypesAllowed<std::remove_pointer_t<FArg>>();
				}
			}
			buffer_iterator(const category_source& ecs) : gatherer<category_source>(ecs) {
				this->m_ecs.template componentTypesAllowed<std::remove_pointer_t<Args>...>();
				static_assert(!std::is_same_v<FArg, rynx::ecs::id*>, "id pointer must be const when iterating buffers!");
				if constexpr (!std::is_same_v<FArg, rynx::ecs::id const*>) {
					this->m_ecs.template componentTypesAllowed<std::remove_pointer_t<FArg>>();
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
							std::apply(call_user_op, entity_category.second->template table_datas<accessType, id_types_t>(*m_typeAliases));
						}
						else {
							auto call_user_op = [size = ids.size(), &op](auto... args) {
								op(size, args...);
							};
							std::apply(call_user_op, entity_category.second->template table_datas<accessType, types_t>(*m_typeAliases));
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
						auto* id_begin = ids.begin();
						auto* id_end = id_begin + ids.size();
						
						if constexpr (is_id_query) {
							auto parameters_tuple = entity_category.second->template table_datas<accessType, components_without_first_t>(*this->m_typeAliases);
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
							auto parameters_tuple = entity_category.second->template table_datas<accessType, components_t>(*this->m_typeAliases);
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
							call_user_op<is_id_query>(std::forward<F>(op), ids, entity_category.second->template table_data<accessType, Args>(*this->m_typeAliases)...);
						}
						else {
							call_user_op<is_id_query>(std::forward<F>(op), ids, entity_category.second->template table_data<accessType, FArg>(*this->m_typeAliases), entity_category.second->template table_data<accessType, Args>(*this->m_typeAliases)...);
						}
					}
				}
			}

			template<typename F> void for_each_partial(range r, F&& op) {
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

						range category_range;

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
							call_user_op_range<is_id_query>(category_range, std::forward<F>(op), ids, entity_category.second->template table_data<accessType, Args>(*this->m_typeAliases)...);
						}
						else {
							call_user_op_range<is_id_query>(category_range, std::forward<F>(op), ids, entity_category.second->template table_data<accessType, FArg>(*this->m_typeAliases), entity_category.second->template table_data<accessType, Args>(*this->m_typeAliases)...);
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

				auto blocker_task = task_context.extend_task_independent([]() {});

				auto& categories = this->m_ecs.categories();
				for (auto&& entity_category : categories) {
					if (entity_category.second->includesAll(this->includeTypes) & entity_category.second->includesNone(this->excludeTypes)) {
						auto& ids = entity_category.second->ids();
						if constexpr (is_id_query) {
							auto barrier = call_user_op_parallel<is_id_query>(std::forward<F>(op), task_context, ids, entity_category.second->template table_data<accessType, Args>(*this->m_typeAliases)...);
							blocker_task.depends_on(barrier);
						}
						else {
							auto barrier = call_user_op_parallel<is_id_query>(std::forward<F>(op), task_context, ids, entity_category.second->template table_data<accessType, FArg>(*this->m_typeAliases), entity_category.second->template table_data<accessType, Args>(*this->m_typeAliases)...);
							blocker_task.depends_on(barrier);
						}
					}
				}
				return blocker_task;
			}

		private:
			template<bool isIdQuery, typename F, typename... Ts>
			static void call_user_op(F&& op, std::vector<id>& ids, Ts* rynx_restrict ... data_ptrs) {
				[[maybe_unused]] auto ids_size = ids.size();
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
			static void call_user_op_range(range iteration_range, F&& op, std::vector<id>& ids, Ts* rynx_restrict ... data_ptrs) {
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

			template<bool isIdQuery, typename F, typename TaskContext, typename... Ts>
			static auto call_user_op_parallel(F&& op, TaskContext&& task_context, std::vector<id>& ids, Ts* rynx_restrict ... data_ptrs) {
				if constexpr (isIdQuery) {
					return task_context.parallel().for_each(0, ids.size()).deferred_work().for_each([op, &ids, args = std::make_tuple(data_ptrs...)](int64_t index) mutable {
						std::apply([&ids, op, index](auto*... ptrs) mutable {
							op(ids[index], ptrs[index]...);
						}, args);
					});
				}
				else {
					return task_context.parallel().for_each(0, ids.size()).deferred_work().for_each([op, args = std::make_tuple(data_ptrs...)](int64_t index) mutable {
						std::apply([op, index](auto... ptrs) mutable {
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
		public:
			entity(category_source& parent, entity_id_t id) : categorySource(&parent), m_id(id) {
				auto categoryAndIndex = categorySource->category_and_index_for(m_id);
				m_entity_category = categoryAndIndex.first;
				m_category_index = categoryAndIndex.second;
			}

			template<typename T> T& get() {
				categorySource->template componentTypesAllowed<T&>();
				rynx_assert(m_entity_category, "referenced entity seems to not exist.");
				return m_entity_category->template table<T>()[m_category_index];
			}

			template<typename T> T& get(rynx::type_index::virtual_type type) {
				categorySource->template componentTypesAllowed<T&>();
				rynx_assert(m_entity_category, "referenced entity seems to not exist.");
				return m_entity_category->template table<T>(type.type_value)[m_category_index];
			}

			template<typename T> T* try_get() {
				categorySource->template componentTypesAllowed<T&>();
				type_id_t typeIndex = categorySource->template typeId<T>();
				rynx_assert(m_entity_category != nullptr, "referenced entity seems to not exist.");
				if (m_entity_category->types().test(typeIndex)) {
					return &m_entity_category->template table<T>(typeIndex)[m_category_index];
				}
				return nullptr;
			}

			template<typename T> T* try_get(rynx::type_index::virtual_type type) {
				categorySource->template componentTypesAllowed<T&>();
				type_id_t typeIndex = type.type_value;
				rynx_assert(m_entity_category != nullptr, "referenced entity seems to not exist.");
				if (m_entity_category->types().test(typeIndex)) {
					return &m_entity_category->template table<T>(typeIndex)[m_category_index];
				}
				return nullptr;
			}

			template<typename... Ts> bool has() const noexcept {
				categorySource->template componentTypesAllowed<std::add_const_t<Ts> ...>();
				rynx_assert(m_entity_category, "referenced entity seems to not exist.");
				return true & (m_entity_category->types().test(categorySource->template typeId<Ts>()) & ...);
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
			}

			template<typename T> void remove(rynx::type_index::virtual_type type) {
				static_assert(allowEditing && sizeof(T) >= 0, "You took this entity from an ecs::view. Removing components like this is not allowed. Use edit_view to remove components instead.");
				categorySource->editor().template type_alias<T>(type).template removeFromEntity<T>(m_id);
			}

			template<typename T> void add(T&& t) {
				static_assert(allowEditing && sizeof(T) >= 0, "You took this entity from an ecs::view. Adding components like this is not allowed. Use edit_view to add components instead.");
				categorySource->editor().template attachToEntity<T>(m_id, std::forward<T>(t));
			}

			template<typename T> void add(T&& t, rynx::type_index::virtual_type type) {
				static_assert(allowEditing && sizeof(T) >= 0, "You took this entity from an ecs::view. Adding components like this is not allowed. Use edit_view to add components instead.");
				categorySource->editor().template type_alias<T>(type).template attachToEntity<T>(m_id, std::forward<T>(t));
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
				type_id_t typeIndex = categorySource->template typeId<T>();
				rynx_assert(m_entity_category != nullptr, "referenced entity seems to not exist.");
				if (m_entity_category->types().test(typeIndex)) {
					return &m_entity_category->template table<T>(typeIndex)[m_category_index];
				}
				return nullptr;
			}
			template<typename... Ts> bool has() const {
				categorySource->template componentTypesAllowed<Ts...>();
				rynx_assert(m_entity_category != nullptr, "referenced entity seems to not exist.");
				return true & (m_entity_category->types().test(categorySource->template typeId<Ts>()) & ...);
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
			~query_t() { rynx_assert(m_consumed, "did you forget to execute query object?"); }
			
			template<typename...Ts> query_t& in() { (inTypes.set(m_ecs.template typeId<std::add_const_t<Ts>>()), ...); return *this; }
			template<typename...Ts> query_t& notIn() { (notInTypes.set(m_ecs.template typeId<std::add_const_t<Ts>>()), ...); return *this; }
			
			query_t& in(rynx::type_index::virtual_type t) { inTypes.set(t.type_value); return *this; }
			query_t& notIn(rynx::type_index::virtual_type t) { notInTypes.set(t.type_value); return *this; }

			template<typename T> query_t& type_alias(type_index::virtual_type mapped_type) {
				m_typeAliases.emplace(m_ecs.template typeId<T>(), mapped_type.type_value);
				return *this;
			}

			template<typename F>
			query_t& for_each(F&& op) {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				entity_iterator<accessType, category_source, decltype(&F::operator())> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(m_typeAliases);
				it.for_each(std::forward<F>(op));
				return *this;
			}

			template<typename F>
			range for_each_partial(range r, F&& op) {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				entity_iterator<accessType, category_source, decltype(&F::operator())> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(m_typeAliases);
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
				
				if constexpr (false) {
					return task_context.extend_task_execute_parallel([op, query_config = *this](TaskContext& task_context) mutable {
						entity_iterator<accessType, category_source, decltype(&F::operator())> it(query_config.m_ecs);
						it.include(std::move(query_config.inTypes));
						it.exclude(std::move(query_config.notInTypes));
						it.type_aliases(query_config.m_typeAliases);
						it.for_each_parallel(task_context, op);
					});
				}
				else {
					entity_iterator<accessType, category_source, decltype(&F::operator())> it(this->m_ecs);
					it.include(std::move(this->inTypes));
					it.exclude(std::move(this->notInTypes));
					it.type_aliases(this->m_typeAliases);
					return it.for_each_parallel(task_context, op);
				}
			}

			template<typename F>
			query_t& for_each_buffer(F&& op) {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				buffer_iterator<accessType, category_source, decltype(&F::operator())> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(m_typeAliases);
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
				it.type_aliases(m_typeAliases);
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
				it.type_aliases(m_typeAliases);
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
				it.type_aliases(m_typeAliases);
				return it.count();
			}

			template<typename T>
			void sort_by() {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				sorter<category_source> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(m_typeAliases);
				it.template sort_categories_by<T>();
			}

			template<typename T>
			void sort_single_step_by() {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				sorter<category_source> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.type_aliases(m_typeAliases);
				it.template sort_buckets_one_step<T>();
			}
		};

		ecs() = default;
		~ecs() {}

		constexpr size_t size() const {
			return m_idCategoryMap.size();
		}

		// should call once per frame. this is not thread safe.
		// if you don't call this, you will see performance go down the toilet.
		void sync_type_index() {
			m_types.sync();
		}

		// used to tell the type index block which types we are going to use before using them.
		// calling this is not required. but types which are not synced to the type index will work slower, if the
		// type index is using local type indexing.
		// you can also call sync_type_index() manually once per frame or at convenient times. when sync is called,
		// no other operation is allowed to hit the type index.
		template<typename...Ts>
		void type_index_using() {
			(m_types.id<Ts>(), ...);
			sync_type_index();
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

			template<typename T> type_id_t typeId() { return m_ecs->typeId<T>(); }
			template<typename...Ts> void componentTypesAllowed() const {};

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
			if (it != m_idCategoryMap.end()) {
				it->second.first->erase(it->second.second, m_idCategoryMap);
			}
		}
		
		// TODO: c++20 Concepts will help here.
		template<typename T>
		void erase(const T& iterable) {
			for (auto&& id : iterable) {
				erase(id);
			}
		}

		void erase(id entityId) { erase(entityId.value); }

		template<typename...Args> void componentTypesAllowed() const {}

		template<typename T> type_id_t type_id_for([[maybe_unused]] const T& t) {
			if constexpr (std::is_same_v<std::remove_cvref_t<T>, rynx::type_index::virtual_type>) {
				return t.type_value;
			}
			else {
				return m_types.id<T>();
			}
		}

		template<typename T> type_id_t type_id_for([[maybe_unused]] const std::vector<T>& t) {
			return m_types.id<T>();
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

		public:
			edit_t(ecs& host) : m_ecs(host) {}

			template<typename T> edit_t& type_alias(type_index::virtual_type mapped_type) {
				m_typeAliases.emplace(m_ecs.m_types.id<T>(), mapped_type.type_value);
				return *this;
			}

			template<typename... Components>
			entity_id_t create(Components&& ... components) {
				entity_id_t id = m_ecs.m_entities.generateOne();
				dynamic_bitset targetCategory;
				(targetCategory.set(mapped_type_id(m_ecs.type_id_for(components))), ...);
				auto category_it = m_ecs.m_categories.find(targetCategory);
				if (category_it == m_ecs.m_categories.end()) { category_it = m_ecs.m_categories.emplace(targetCategory, std::make_unique<entity_category>(targetCategory, &m_ecs.m_types)).first; }
				category_it->second->insertNew(m_typeAliases, id, std::forward<Components>(components)...);
				m_ecs.m_idCategoryMap.emplace(id, std::make_pair(category_it->second.get(), index_t(category_it->second->size() - 1)));
				return id;
			}

			// TODO: The api for tags is super unclear for user. Should have some kind of ..
			//       create_n(ids, vector<Ts>).with_tags<Tag1, Tag2...>(); // style format.
			template<typename... Tags, typename... Components>
			std::vector<entity_id_t> create_n(std::vector<Components>& ... components) {
				rynx_assert((components.size() & ...) == (components.size() | ...), "components vector sizes do not match!");
				std::vector<entity_id_t> ids(rynx::first_of(components...).size());

				// TODO: generate_n might be more efficient. benchmark if this shows up in profiler.
				for (entity_id_t& id : ids) {
					id = m_ecs.m_entities.generateOne();
				}

				// target category is the same for all entities created in this call.
				dynamic_bitset targetCategory;
				(targetCategory.set(mapped_type_id(m_ecs.type_id_for(components))), ...);
				(targetCategory.set(mapped_type_id(m_ecs.m_types.id<Tags>())), ...);
				auto category_it = m_ecs.m_categories.find(targetCategory);
				if (category_it == m_ecs.m_categories.end()) { category_it = m_ecs.m_categories.emplace(targetCategory, std::make_unique<entity_category>(targetCategory, &m_ecs.m_types)).first; }

				auto first_index = category_it->second->size();
				category_it->second->insertNew<Tags...>(m_typeAliases, ids, components...);
				for (size_t i = 0; i < ids.size(); ++i) {
					m_ecs.m_idCategoryMap.emplace(ids[i], std::make_pair(category_it->second.get(), index_t(first_index + i)));
				}
				return ids;
			}

			template<typename... Components>
			edit_t& attachToEntity(entity_id_t id, Components&& ... components) {
				auto it = m_ecs.m_idCategoryMap.find(id);
				rynx_assert(it != m_ecs.m_idCategoryMap.end(), "attachToEntity called for entity that does not exist.");
				const dynamic_bitset& initialTypes = it->second.first->types();
				dynamic_bitset resultTypes = initialTypes;
				(resultTypes.set(mapped_type_id(m_ecs.type_id_for(components))), ...);

				auto destinationCategoryIt = m_ecs.m_categories.find(resultTypes);
				if (destinationCategoryIt == m_ecs.m_categories.end()) {
					destinationCategoryIt = m_ecs.m_categories.emplace(resultTypes, std::make_unique<entity_category>(resultTypes, &m_ecs.m_types)).first;
					destinationCategoryIt->second->copyTypesFrom(it->second.first); // NOTE: Must make copies of table types for tables for which we don't know the type in this context.
				}

				// first migrate the old stuff, then apply on top of that what was given (in case these types overlap).
				destinationCategoryIt->second->migrateEntity(
					initialTypes, // types moved
					it->second.first, // source category
					it->second.second, // source index
					m_ecs.m_idCategoryMap
				);

				destinationCategoryIt->second->insertComponents(m_typeAliases, std::forward<Components>(components)...);
				return *this;
			}

			template<typename... Components>
			edit_t& removeFromEntity(entity_id_t id) {
				auto it = m_ecs.m_idCategoryMap.find(id);
				rynx_assert(it != m_ecs.m_idCategoryMap.end(), "removeFromEntity called for entity that does not exist.");
				const dynamic_bitset& initialTypes = it->second.first->types();
				dynamic_bitset resultTypes = initialTypes;
				(resultTypes.reset(mapped_type_id(m_ecs.m_types.id<Components>())), ...);

				auto destinationCategoryIt = m_ecs.m_categories.find(resultTypes);
				if (destinationCategoryIt == m_ecs.m_categories.end()) {
					destinationCategoryIt = m_ecs.m_categories.emplace(resultTypes, std::make_unique<entity_category>(resultTypes, &m_ecs.m_types)).first;
					destinationCategoryIt->second->copyTypesFrom(it->second.first, resultTypes);
				}

				it->second.first->eraseComponentsFromIndex<Components...>(m_typeAliases, it->second.second);
				destinationCategoryIt->second->migrateEntity(resultTypes, it->second.first, it->second.second, m_ecs.m_idCategoryMap);
				return *this;
			}

			edit_t& removeFromEntity(entity_id_t id, rynx::type_index::virtual_type t) {
				auto it = m_ecs.m_idCategoryMap.find(id);
				rynx_assert(it != m_ecs.m_idCategoryMap.end(), "removeFromEntity called for entity that does not exist.");
				const dynamic_bitset& initialTypes = it->second.first->types();
				dynamic_bitset resultTypes = initialTypes;
				resultTypes.reset(t.type_value);

				auto destinationCategoryIt = m_ecs.m_categories.find(resultTypes);
				if (destinationCategoryIt == m_ecs.m_categories.end()) {
					destinationCategoryIt = m_ecs.m_categories.emplace(resultTypes, std::make_unique<entity_category>(resultTypes, &m_ecs.m_types)).first;
					destinationCategoryIt->second->copyTypesFrom(it->second.first, resultTypes);
				}

				// normally we would need to erase the components from source category entity first.
				// but no need to erase any actual data from actual vectors here, since virtual types do not describe actual data types.
				destinationCategoryIt->second->migrateEntity(
					resultTypes,
					it->second.first,
					it->second.second,
					m_ecs.m_idCategoryMap
				);
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
		edit_t attachToEntity(entity_id_t id, Components&& ... components) {
			return edit_t(*this).attachToEntity(id, std::forward<Components>(components)...);
		}

		template<typename... Components>
		edit_t removeFromEntity(entity_id_t id) {
			return edit_t(*this).removeFromEntity<Components...>(id);
		}

		edit_t removeFromEntity(entity_id_t id, rynx::type_index::virtual_type t) {
			return edit_t(*this).removeFromEntity(id, t);
		}

		template<typename... Components> ecs& attachToEntity(id id, Components&& ... components) { attachToEntity(id.value, std::forward<Components>(components)...); return *this; }
		template<typename... Components> ecs& removeFromEntity(id id) { removeFromEntity<Components...>(id.value); return *this; }

		rynx::type_index::virtual_type create_virtual_type() { return m_types.create_virtual_type(); }

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
			auto& entity_category_map() { return m_ecs->m_idCategoryMap; }

		protected:
			template<typename...Args> void componentTypesAllowed() const {
				static_assert(typesAllowed<Args...>(), "Your ecs view does not have access to one of requested types.");
				static_assert(typesConstCorrect<Args...>(), "You promised to access type in only const context.");
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

			template<typename T> type_id_t typeId() const { return m_ecs->template typeId<T>(); }
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
				return edit_t(*m_ecs);
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
				this->template componentTypesAllowed<Components...>();
				this->m_ecs->template removeFromEntity<Components...>(id);
				return *this;
			}
			template<typename... Components> edit_view& removeFromEntity(id id) { return removeFromEntity<Components...>(id.value); }

			template<typename...Args> operator edit_view<Args...>() {
				static_assert((view::type_verify<Args>() && ...), "all requested types must be present in parent view");
				return edit_view<Args...>(this->m_ecs);
			}
		};

		template<typename...Args> operator view<Args...>() const { return view<Args...>(this); }
		template<typename...Args> operator edit_view<Args...>() const { return edit_view<Args...>(this); }
	};

}
