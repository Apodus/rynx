#pragma once

#include <rynx/tech/dynamic_bitset.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/type_index.hpp>

#include <rynx/tech/profiling.hpp>

#include <vector>
#include <type_traits>
#include <tuple>
#include <algorithm>

template<typename T> struct remove_first_type {};
template<typename T, typename... Ts> struct remove_first_type<std::tuple<T, Ts...>> { using type = std::tuple<Ts...>; };

template<bool condition, typename TupleType> struct remove_front_if {
	using type = std::conditional_t<condition, typename remove_first_type<TupleType>::type, TupleType>;
};

template<typename T, typename... Others> constexpr bool isAny() { return false || (std::is_same_v<std::remove_reference_t<T>, std::remove_reference_t<Others>> || ...); }

namespace rynx {
	class ecs {
	public:
		using type_id_t = uint64_t;
		using entity_id_t = uint64_t;
		using index_t = uint32_t;

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

		type_index m_types;
		entity_index m_entities;

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
			// virtual itable* deepCopy() const = 0;
			virtual void copyTableTypeTo(type_id_t typeId, std::vector<std::unique_ptr<itable>>& targetTables) = 0;
			virtual void moveFromIndexTo(index_t index, itable* dst) = 0;
		};

		template<typename T>
		class component_table : public itable {
		public:
			virtual ~component_table() {}

			void insert(T&& t) { m_data.emplace_back(std::forward<T>(t)); }
			template<typename...Ts> void emplace_back(Ts&& ... ts) { m_data.emplace_back(std::forward<Ts>(ts)...); }
			virtual void erase(entity_id_t id) override {
				m_data[id] = std::move(m_data.back());
				m_data.pop_back();
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
				if(index + 1 != m_data.size())
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

			std::pair<migrate_move, migrate_move> erase(index_t index) {
				for (auto&& table : m_tables) {
					if(table)
						table->erase(index);
				}

				rynx_assert(index < m_ids.size(), "out of bounds");
				auto erasedEntityId = m_ids[index];
				m_ids[index] = std::move(m_ids.back());
				m_ids.pop_back();
				
				// NOTE: This approach of returning what was done and require caller to react is a little bit sad.
				//       We could instead pass the datastructures in here and do the modifications right here? or something.
				if(index != m_ids.size())
					return { migrate_move(erasedEntityId.value, 0, nullptr), migrate_move(m_ids[index].value, index, this) };
				else
					return { migrate_move(erasedEntityId.value, 0, nullptr), migrate_move(rynx::ecs::entity_index::InvalidId, index, this) };
			}

			template<typename...Components> size_t insertNew(entity_id_t id, Components&& ... components) {
				(table<Components>().emplace_back(std::forward<Components>(components)), ...);
				size_t index = m_ids.size();
				m_ids.emplace_back(id);
				return index;
			}

			// these always operate on the last entity, when called directly on a category.
			template<typename...Components> void insertComponents(Components&& ... components) {
				(table<Components>().emplace_back(std::forward<Components>(components)), ...);
			}

			template<typename Component> void eraseComponentFromIndex(index_t index) {
				auto& t = table<Component>();
				t[index] = std::move(t.back());
				t.pop_back();
			}
			template<typename...Components> void eraseComponentsFromIndex(index_t index) { (eraseComponentFromIndex<Components>(index), ...); }

			void copyTypesFrom(entity_category* other, const dynamic_bitset& types) {
				type_id_t typeId = types.nextOne(0);
				while (typeId != dynamic_bitset::npos) {
					other->m_tables[typeId]->copyTableTypeTo(typeId, m_tables);
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

			template<DataAccess accessType, typename ComponentsTuple> auto table_datas() {
				return getTableDatas<accessType, ComponentsTuple>()(*this);
			}

			template<DataAccess accessType, typename T> auto table_data() {
				return getTableData<accessType, T>()(*this);
			}

			const dynamic_bitset& types() const { return m_types; }

			std::pair<migrate_move, migrate_move> migrateEntity(const dynamic_bitset& types, entity_category* source, index_t source_index) {
				type_id_t typeId = types.nextOne(0);
				while (typeId != dynamic_bitset::npos) {
					source->m_tables[typeId]->moveFromIndexTo(source_index, m_tables[typeId].get());
					typeId = types.nextOne(typeId + 1);
				}

				m_ids.emplace_back(source->m_ids[source_index]);
				source->m_ids[source_index] = source->m_ids.back();
				source->m_ids.pop_back();

				// NOTE: This is a bit unclean solution. If there's time, see if this could be cleaned up.
				//       If we had access to the mapping within this function, we could avoid this silliness of returning 0, 0, nullptr.
				if (source->m_ids.size() == source_index) {
					return std::pair<migrate_move, migrate_move>(
						migrate_move(m_ids.back().value, index_t(m_ids.size() - 1), this),
						migrate_move(0, 0, nullptr)
					);
				}
				else {
					return std::pair<migrate_move, migrate_move>(
						migrate_move(m_ids.back().value, index_t(m_ids.size() - 1), this),
						migrate_move(source->m_ids[source_index].value, source_index, source)
					);
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

			template<typename T> const auto& table(type_id_t typeIndex) const { return const_cast<entity_category*>(this)->table<T>(typeIndex); }
			template<typename T> const auto& table() const { return const_cast<entity_category*>(this)->table<T>(); }

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
				auto operator()(entity_category& categorySource) {
					if constexpr (accessType == DataAccess::Const) {
						return std::make_tuple(const_cast<const entity_category&>(categorySource).table<Ts>().data()...);
					}
					else {
						return std::make_tuple(categorySource.table<Ts>().data()...);
					}
				}
			};

			template<DataAccess accessType, typename T> struct getTableData {
				auto operator()(entity_category& categorySource) {
					if constexpr (accessType == DataAccess::Const) {
						return const_cast<const entity_category&>(categorySource).table<T>().data();
					}
					else {
						return categorySource.table<T>().data();
					}
				}
			};

			dynamic_bitset m_types;
			std::vector<std::unique_ptr<itable>> m_tables;
			std::vector<id> m_ids;
			type_index* m_typeIndex;
		};

		// TODO: is is ok to have the parallel implementation baked into iterator?
		template<DataAccess accessType, typename category_source, typename F> class iterator {};
		template<DataAccess accessType, typename category_source, typename Ret, typename Class, typename FArg, typename... Args>
		class iterator<accessType, category_source, Ret(Class::*)(FArg, Args...) const> {
		public:
			iterator(category_source& ecs) : m_ecs(ecs) { m_ecs.template componentTypesAllowed<Args...>(); }
			iterator(const category_source& ecs) : m_ecs(const_cast<category_source&>(ecs)) { m_ecs.template componentTypesAllowed<Args...>(); }

			template<typename F> void for_each(F&& op) {
				constexpr bool is_id_query = std::is_same_v<FArg, rynx::ecs::id>;
				if constexpr (!is_id_query) {
					unpack_types<FArg>();
				}
				unpack_types<Args...>();
				
				auto& categories = m_ecs.categories();
				for (auto&& entity_category : categories) {
					if (entity_category.second->includesAll(includeTypes) & entity_category.second->includesNone(excludeTypes)) {
						auto& ids = entity_category.second->ids();
						if constexpr (is_id_query) {
							call_user_op<is_id_query>(std::forward<F>(op), ids, entity_category.second->template table_data<accessType, Args>()...);
						}
						else {
							call_user_op<is_id_query>(std::forward<F>(op), ids, entity_category.second->template table_data<accessType, FArg>(), entity_category.second->template table_data<accessType, Args>()...);
						}
					}
				}
			}

			template<typename F, typename TaskContext> void for_each_parallel(TaskContext&& task_context, F&& op) {
				constexpr bool is_id_query = std::is_same_v<FArg, rynx::ecs::id>;
				if constexpr (!is_id_query) {
					unpack_types<FArg>();
				}
				unpack_types<Args...>();

				auto& categories = m_ecs.categories();
				for (auto&& entity_category : categories) {
					if (entity_category.second->includesAll(includeTypes) & entity_category.second->includesNone(excludeTypes)) {
						auto& ids = entity_category.second->ids();
						if constexpr (is_id_query) {
							call_user_op_parallel<is_id_query>(std::forward<F>(op), task_context, ids, entity_category.second->template table_data<accessType, Args>()...);
						}
						else {
							call_user_op_parallel<is_id_query>(std::forward<F>(op), task_context, ids, entity_category.second->template table_data<accessType, FArg>(), entity_category.second->template table_data<accessType, Args>()...);
						}
					}
				}
			}

			iterator& include(dynamic_bitset inTypes) { includeTypes = std::move(inTypes); return *this; }
			iterator& exclude(dynamic_bitset notInTypes) { excludeTypes = std::move(notInTypes); return *this; }

		private:
			template<bool isIdQuery, typename F>
			static void call_user_op(F&& op, std::vector<id>& ids) {
				if constexpr (isIdQuery) {
					std::for_each(ids.data(), ids.data() + ids.size(), [=](id entityId) mutable {
						op(entityId);
					});
				}
				else {
					for (size_t i = 0; i < ids.size(); ++i)
						op();
				}
			}
			
			template<bool isIdQuery, typename F, typename T_first, typename... Ts>
			static void call_user_op(F&& op, std::vector<id>& ids, T_first * rynx_restrict data_ptrs_first, Ts * rynx_restrict ... data_ptrs) {
				if constexpr (isIdQuery) {
					std::for_each(ids.data(), ids.data() + ids.size(), [=](id entityId) mutable {
						op(entityId, *data_ptrs_first++, (*data_ptrs++)...);
					});
				}
				else {
					std::for_each(data_ptrs_first, data_ptrs_first + ids.size(), [=](T_first& a) mutable {
						op(a, (*data_ptrs++)...);
					});
				}
			}

			template<bool isIdQuery, typename F, typename TaskContext, typename... Ts>
			static void call_user_op_parallel(F&& op, TaskContext&& task_context, std::vector<id>& ids, Ts* rynx_restrict ... data_ptrs) {
				if constexpr (isIdQuery) {
					task_context & task_context.parallel().for_each(0, ids.size(), [op, &ids, args = std::make_tuple(data_ptrs...)](int64_t index) {
						std::apply([&, index](auto... ptrs) {
							op(ids[index], ptrs[index]...);
						}, args);
					});
				}
				else {
					task_context & task_context.parallel().for_each(0, ids.size(), [op, &ids, args = std::make_tuple(data_ptrs...)](int64_t index) {
						std::apply([&, index](auto... ptrs) {
							op(ptrs[index]...);
						}, args);
					});
				}
			}

			template<typename TupleType, size_t...Is> void unpack_types(std::index_sequence<Is...>) { (includeTypes.set(m_ecs.template typeId<typename std::tuple_element<Is, TupleType>::type>()), ...); }
			template<typename... Ts> void unpack_types() { (includeTypes.set(m_ecs.template typeId<Ts>()), ...); }

			dynamic_bitset includeTypes;
			dynamic_bitset excludeTypes;
			category_source& m_ecs;
		};

		// support for mutable lambdas / non-const member functions
		template<DataAccess accessType, typename category_source, typename Ret, typename Class, typename FArg, typename... Args>
		class iterator<accessType, category_source, Ret(Class::*)(FArg, Args...)> : public iterator<accessType, category_source, Ret(Class::*)(FArg, Args...) const> {
		public:
			iterator(category_source& ecs) : iterator<accessType, category_source, Ret(Class::*)(FArg, Args...) const>(ecs) {}
			iterator(const category_source& ecs) : iterator<accessType, category_source, Ret(Class::*)(FArg, Args...) const>(ecs) {}
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
				categorySource->template componentTypesAllowed<T>();
				rynx_assert(m_entity_category, "referenced entity seems to not exist.");
				return m_entity_category->template table<T>()[m_category_index];
			}

			template<typename T> T* try_get() {
				categorySource->template componentTypesAllowed<T>();
				type_id_t typeIndex = categorySource->template typeId<T>();
				rynx_assert(m_entity_category != nullptr, "referenced entity seems to not exist.");
				if (m_entity_category->types().test(typeIndex)) {
					return &m_entity_category->template table<T>(typeIndex)[m_category_index];
				}
				return nullptr;
			}

			template<typename... Ts> bool has() const {
				categorySource->template componentTypesAllowed<std::add_const_t<Ts> ...>();
				rynx_assert(m_entity_category, "referenced entity seems to not exist.");
				return true && (m_entity_category->types().test(categorySource->template typeId<Ts>()) && ...);
			}

			template<typename... Ts> void remove() {
				static_assert(allowEditing && sizeof...(Ts) >= 0, "You took this entity from an ecs::view. Removing components like this is not allowed. Use edit_view to remove components instead.");
				categorySource->template removeFromEntity<Ts...>(m_id);
			}
			template<typename T> void add(T&& t) {
				static_assert(allowEditing && sizeof(T) >= 0, "You took this entity from an ecs::view. Adding components like this is not allowed. Use edit_view to add components instead.");
				categorySource->template attachToEntity<T>(m_id, std::forward<T>(t));
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
				return true && (m_entity_category->types().test(categorySource->template typeId<Ts>()) && ...);
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
			category_source& m_ecs;
			bool m_consumed = false;

		public:
			query_t(category_source& ecs) : m_ecs(ecs) {}
			query_t(const category_source& ecs_) : m_ecs(const_cast<category_source&>(ecs_)) {}
			~query_t() { rynx_assert(m_consumed, "did you forgete to execute query object?"); }
			
			template<typename...Ts> query_t& in() { (inTypes.set(m_ecs.template typeId<std::add_const_t<Ts>>()), ...); return *this; }
			template<typename...Ts> query_t& notIn() { (notInTypes.set(m_ecs.template typeId<std::add_const_t<Ts>>()), ...); return *this; }

			template<typename F>
			query_t& execute(F&& op) {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				iterator<accessType, category_source, decltype(&F::operator())> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.for_each(std::forward<F>(op));
				return *this;
			}

			template<typename F, typename TaskContext>
			query_t& execute_parallel(TaskContext&& task_context, F&& op) {
				rynx_assert(!m_consumed, "same query object cannot be executed twice.");
				m_consumed = true;
				iterator<accessType, category_source, decltype(&F::operator())> it(m_ecs);
				it.include(std::move(inTypes));
				it.exclude(std::move(notInTypes));
				it.for_each_parallel(std::forward<TaskContext>(task_context), std::forward<F>(op));
				return *this;
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

		bool exists(entity_id_t id) const { return m_idCategoryMap.find(id) != m_idCategoryMap.end(); }
		bool exists(id id) const { return exists(id.value); }

		entity<ecs, true> operator[](entity_id_t id) { return entity<ecs, true>(*this, id); }
		entity<ecs, true> operator[](id id) { return entity<ecs, true>(*this, id.value); }
		const_entity<ecs> operator[](entity_id_t id) const { return const_entity<ecs>(*this, id); }
		const_entity<ecs> operator[](id id) const { return const_entity<ecs>(*this, id.value); }

		template<typename F>
		void for_each(F&& op) {
			rynx_profile("Ecs", "for_each");
			iterator<DataAccess::Mutable, ecs, decltype(&F::operator())> it(*this);
			it.for_each(std::forward<F>(op));
		}

		template<typename F>
		void for_each(F&& op) const {
			rynx_profile("Ecs", "for_each");
			iterator<DataAccess::Const, ecs, decltype(&F::operator())> it(*this);
			it.for_each(std::forward<F>(op));
		}

		template<typename F, typename TaskContext>
		void for_each_parallel(TaskContext&& task_context, F&& op) {
			rynx_profile("Ecs", "for_each_parallel");
			iterator<DataAccess::Mutable, view, decltype(&F::operator())> it(*this);
			it.for_each_parallel(std::forward<TaskContext>(task_context), std::forward<F>(op));
		}

		template<typename F, typename TaskContext>
		void for_each_parallel(TaskContext&& task_context, F&& op) const {
			rynx_profile("Ecs", "for_each_parallel");
			iterator<DataAccess::Const, view, decltype(&F::operator())> it(*this);
			it.for_each_parallel(std::forward<TaskContext>(task_context), std::forward<F>(op));
		}

		query_t<DataAccess::Mutable, ecs> query() { return query_t<DataAccess::Mutable, ecs>(*this); }
		query_t<DataAccess::Const, ecs> query() const { return query_t<DataAccess::Const, ecs>(*this); }

		std::pair<entity_category*, index_t> category_and_index_for(entity_id_t id) const {
			auto it = m_idCategoryMap.find(id);
			rynx_assert(it != m_idCategoryMap.end(), "requesting category and index for an entity id that does not exist");
			return it->second;
		}

		void erase(entity_id_t entityId) {
			auto it = m_idCategoryMap.find(entityId);
			if (it != m_idCategoryMap.end()) {
				auto operations = it->second.first->erase(it->second.second);
				m_idCategoryMap.erase(operations.first.id);
				if (operations.second.id != entity_index::InvalidId) {
					m_idCategoryMap[operations.second.id] = { operations.second.new_category, operations.second.newIndex };
				}
			}
		}

		void erase(id entityId) { erase(entityId.value); }

		template<typename...Args> void componentTypesAllowed() const {}

		template<typename... Components>
		entity_id_t create(Components&& ... components) {
			entity_id_t id = m_entities.generateOne();
			dynamic_bitset targetCategory;
			(targetCategory.set(m_types.id<Components>()), ...);
			auto category_it = m_categories.find(targetCategory);
			if (category_it == m_categories.end()) { category_it = m_categories.emplace(targetCategory, std::make_unique<entity_category>(targetCategory, &m_types)).first; }
			category_it->second->insertNew(id, std::forward<Components>(components)...);
			m_idCategoryMap.emplace(id, std::make_pair(category_it->second.get(), index_t(category_it->second->size() - 1)));
			return id;
		}

		template<typename... Components>
		ecs& attachToEntity(entity_id_t id, Components&& ... components) {
			auto it = m_idCategoryMap.find(id);
			rynx_assert(it != m_idCategoryMap.end(), "attachToEntity called for entity that does not exist.");
			const dynamic_bitset& initialTypes = it->second.first->types();
			dynamic_bitset resultTypes = initialTypes;
			(resultTypes.set(m_types.id<Components>()), ...);

			auto destinationCategoryIt = m_categories.find(resultTypes);
			if (destinationCategoryIt == m_categories.end()) {
				destinationCategoryIt = m_categories.emplace(resultTypes, std::make_unique<entity_category>(resultTypes, &m_types)).first;
				destinationCategoryIt->second->copyTypesFrom(it->second.first); // NOTE: Must make copies of table types for tables for which we don't know the type in this context.
			}

			// first migrate the old stuff, then apply on top of that what was given (in case these types overlap).
			auto changePair = destinationCategoryIt->second->migrateEntity(initialTypes, it->second.first, it->second.second);
			//                                                             ^types moved, ^source category, ^source index
			destinationCategoryIt->second->insertComponents(std::forward<Components>(components)...);
			m_idCategoryMap.find(changePair.first.id)->second = { changePair.first.new_category, changePair.first.newIndex };
			if (changePair.second.new_category)
				m_idCategoryMap.find(changePair.second.id)->second = { changePair.second.new_category, changePair.second.newIndex };

			return *this;
		}

		template<typename... Components>
		ecs& removeFromEntity(entity_id_t id) {
			auto it = m_idCategoryMap.find(id);
			rynx_assert(it != m_idCategoryMap.end(), "removeFromEntity called for entity that does not exist.");
			const dynamic_bitset& initialTypes = it->second.first->types();
			dynamic_bitset resultTypes = initialTypes;
			(resultTypes.reset(m_types.id<Components>()), ...);

			auto destinationCategoryIt = m_categories.find(resultTypes);
			if (destinationCategoryIt == m_categories.end()) {
				destinationCategoryIt = m_categories.emplace(resultTypes, std::make_unique<entity_category>(resultTypes, &m_types)).first;
				destinationCategoryIt->second->copyTypesFrom(it->second.first, resultTypes);
			}

			it->second.first->eraseComponentsFromIndex<Components...>(it->second.second);
			auto changePair = destinationCategoryIt->second->migrateEntity(resultTypes, it->second.first, it->second.second);

			m_idCategoryMap.find(changePair.first.id)->second = { changePair.first.new_category, changePair.first.newIndex };
			if (changePair.second.new_category)
				m_idCategoryMap.find(changePair.second.id)->second = { changePair.second.new_category, changePair.second.newIndex };
			return *this;
		}


		template<typename... Components> ecs& attachToEntity(id id, Components&& ... components) { attachToEntity(id.value, std::forward<Components>(components)...); return *this; }
		template<typename... Components> ecs& removeFromEntity(id id) { removeFromEntity<Components...>(id.value); return *this; }

	private:
		auto& categories() { return m_categories; }
		const auto& categories() const { return m_categories; }

	public:
		// ecs view that operates on Args types only. generates compile errors if trying to access any other data.
		// also does not allow creating new components, removing existing components or creating new entities.
		// if you need to do that, use edit_view instead.
		template<typename...TypeConstraints>
		class view {
			template<typename T, bool> friend class rynx::ecs::entity;
			template<typename T> friend class rynx::ecs::const_entity;

			template<DataAccess accessType, typename category_source> friend class rynx::ecs::query_t;
			template<DataAccess accessType, typename category_source, typename F> friend class rynx::ecs::iterator;

			template<typename T> static constexpr bool typeAllowed() { return isAny<std::remove_const_t<T>, id, std::remove_const_t<TypeConstraints>...>(); }
			template<typename T> static constexpr bool typeConstCorrect() {
				if constexpr (std::is_reference_v<T>) {
					if constexpr (std::is_const_v<T>)
						return isAny<T, id, std::add_const_t<TypeConstraints>...>(); // user must have requested any access to type (read/write both acceptable).
					else
						return isAny<T, id, TypeConstraints...>(); // verify user has requested a write access.
				}
				else {
					// user is taking a copy, so it is always const with regards to what we have stored.
					return isAny<std::remove_const_t<T>, id, std::remove_const_t<TypeConstraints>...>();
				}
			}

			template<typename...Args> static constexpr bool typesAllowed() { return true && (typeAllowed<std::remove_reference_t<Args>>() && ...); }
			template<typename...Args> static constexpr bool typesConstCorrect() { return true && (typeConstCorrect<std::remove_reference_t<Args>>() && ...); }

		protected:
			template<typename...Args> void componentTypesAllowed() const {
				static_assert(typesAllowed<Args...>(), "Your ecs view does not have access to one of requested types.");
				static_assert(typesConstCorrect<Args...>(), "You promised to access type in only const context.");
			}

		public:
			view(const rynx::ecs* ecs) : m_ecs(const_cast<rynx::ecs*>(ecs)) {}

			template<typename F>
			void for_each(F&& op) {
				rynx_profile("Ecs", "for_each");
				iterator<DataAccess::Mutable, view, decltype(&F::operator())> it(*this);
				it.for_each(std::forward<F>(op));
			}

			template<typename F>
			void for_each(F&& op) const {
				rynx_profile("Ecs", "for_each");
				iterator<DataAccess::Const, view, decltype(&F::operator())> it(*this);
				it.for_each(std::forward<F>(op));
			}

			template<typename F, typename TaskContext>
			void for_each_parallel(TaskContext&& task_context, F&& op) {
				rynx_profile("Ecs", "for_each");
				iterator<DataAccess::Mutable, view, decltype(&F::operator())> it(*this);
				it.for_each_parallel(std::forward<TaskContext>(task_context), std::forward<F>(op));
			}

			template<typename F, typename TaskContext>
			void for_each_parallel(TaskContext&& task_context, F&& op) const {
				rynx_profile("Ecs", "for_each");
				iterator<DataAccess::Const, view, decltype(&F::operator())> it(*this);
				it.for_each_parallel(std::forward<TaskContext>(task_context), std::forward<F>(op));
			}

			bool exists(entity_id_t id) const { return m_ecs->exists(id); }
			bool exists(id id) const { return exists(id.value); }

			entity<view, false> operator[](entity_id_t id) { return entity<view, false>(*this, id); }
			entity<view, false> operator[](id id) { return entity<view, false>(*this, id.value); }

			const_entity<view> operator[](entity_id_t id) const { return const_entity<view>(*this, id); }
			const_entity<view> operator[](id id) const { return const_entity<view>(*this, id.value); }

			query_t<DataAccess::Mutable, view> query() { return query_t<DataAccess::Mutable, view>(*this); }
			query_t<DataAccess::Const, view> query() const { return query_t<DataAccess::Const, view>(*this); }

		private:
			auto& categories() { return m_ecs->categories(); }
			const auto& categories() const { return m_ecs->categories(); }

			template<typename T> type_id_t typeId() const { return m_ecs->template typeId<T>(); }
			auto category_and_index_for(entity_id_t id) const { return m_ecs->category_and_index_for(id); }

		public:
			template<typename...Args> operator view<Args...>() const {
				static_assert((isAny<Args, TypeConstraints...>() && ...), "all requested types must be present in parent view");
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

			const_entity<view> operator[](entity_id_t id) const { return const_entity<view>(*this, id); }
			const_entity<view> operator[](id id) const { return const_entity<view>(*this, id.value); }

			template<typename... Components>
			entity_id_t create(Components&& ... components) {
				componentTypesAllowed<Components...>();
				return this->m_ecs->create(std::forward<Components>(components)...);
			}

			template<typename... Components>
			edit_view& attachToEntity(entity_id_t id, Components&& ... components) {
				componentTypesAllowed<Components...>();
				this->m_ecs->attachToEntity(id, std::forward<Components>(components)...);
				return *this;
			}
			template<typename... Components> edit_view& attachToEntity(id id, Components&& ... components) { return attachToEntity(id.value, std::forward<Components>(components)...); }

			template<typename... Components>
			edit_view& removeFromEntity(entity_id_t id) {
				componentTypesAllowed<Components...>();
				this->m_ecs->template removeFromEntity<Components...>(id);
				return *this;
			}
			template<typename... Components> edit_view& removeFromEntity(id id) { return removeFromEntity<Components...>(id.value); }

			template<typename...Args> operator edit_view<Args...>() const {
				static_assert((isAny<Args, TypeConstraints...>() && ...), "all requested types must be present in parent view");
				return edit_view<Args...>(this);
			}
		};

		template<typename...Args> operator view<Args...>() const { return view<Args...>(this); }
		template<typename...Args> operator edit_view<Args...>() const { return edit_view<Args...>(this); }

	private:
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

		template<typename T> type_id_t typeId() const { return m_types.template id<T>(); }

		unordered_map<entity_id_t, std::pair<entity_category*, index_t>> m_idCategoryMap;
		unordered_map<dynamic_bitset, std::unique_ptr<entity_category>, bitset_hash> m_categories;
	};

}
