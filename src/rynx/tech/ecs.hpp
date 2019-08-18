#pragma once

#include <rynx/tech/dynamic_bitset.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/type_index.hpp>

// #include <Core/Profiling.h>

#include <vector>
#include <type_traits>
#include <tuple>

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

		// NOTE: Since we are using chunks now, it really doesn't matter how far apart the entity ids are.
		//       Reusing ids is thus very much not important, unless we get to the point of overflow.
		//       But still it would be easier to "reset" entity_index between levels or something.
		class entity_index {
		public:
			entity_id_t generateOne() {
				/*
				if (!freeIds.empty()) {
					auto id = freeIds.back();
					freeIds.pop_back();
					return id;
				}
				*/
				return ++m_nextId;
			} // zero is never generated. we can use that as InvalidId.

			void entityKilled(entity_id_t /* entityId */) {
				// freeIds.emplace_back(entityId);
			}

			static constexpr entity_id_t InvalidId = 0;
		private:
			entity_id_t m_nextId = 0;
			// std::vector<entity_id_t> freeIds;
		};

		type_index types;
		entity_index entities;

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
			virtual void copyTableTypeTo(type_id_t typeId, unordered_map<type_id_t, std::unique_ptr<itable>>& targetTables) = 0;
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

			// NOTE: This is required for moving entities between chunks reliably.
			virtual void copyTableTypeTo(type_id_t typeId, unordered_map<type_id_t, std::unique_ptr<itable>>& targetTables) override {
				targetTables.emplace(typeId, std::make_unique<component_table>());
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

			size_t size() const { return m_data.size(); }

			T& operator[](index_t index) { return m_data[index]; }
			const T& operator[](index_t index) const { return m_data[index]; }

		private:
			std::vector<std::remove_reference_t<T>> m_data;
		};

		class chunk;
		struct migrate_move {
			migrate_move(entity_id_t id_, index_t newIndex_, chunk* newChunk_) : id(id_), newIndex(newIndex_), newChunk(newChunk_) {}
			entity_id_t id;
			index_t newIndex;
			chunk* newChunk;
		};

		class chunk {
		public:
			chunk(dynamic_bitset types, type_index* typeIndex) : m_types(std::move(types)), m_typeIndex(typeIndex) {}

			// TODO: this can be optimized to not reserve memory. operate and compare in dynamic_bitset.
			bool includesAll(const dynamic_bitset& types) const { return (m_types & types) == types; }
			bool includesNone(const dynamic_bitset& types) const { return (m_types & types) == dynamic_bitset(); }

			std::pair<migrate_move, migrate_move> erase(index_t index) {
				for (auto&& table : m_tables) {
					table.second->erase(index);
				}
				auto erasedEntityId = m_ids[index];
				m_ids[index] = m_ids.back();
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

			// these always operate on the last entity.
			template<typename...Components> void insertComponents(Components&& ... components) {
				(table<Components>().emplace_back(std::forward<Components>(components)), ...);
			}

			template<typename Component> void eraseComponentFromIndex(index_t index) {
				auto& t = table<Component>();
				t[index] = t.back();
				t.pop_back();
			}
			template<typename...Components> void eraseComponentsFromIndex(index_t index) { (eraseComponentFromIndex<Components>(index), ...); }

			void copyTypesFrom(chunk* other, const dynamic_bitset& types) {
				type_id_t typeId = types.nextOne(0);
				while (typeId != dynamic_bitset::npos) {
					other->m_tables.find(typeId)->second->copyTableTypeTo(typeId, m_tables);
					typeId = types.nextOne(typeId + 1);
				}
			}
			void copyTypesTo(chunk* other, const dynamic_bitset& types) { other->copyTypesFrom(this, types); }

			void copyTypesFrom(chunk* other) { for (auto&& table : other->m_tables) table.second->copyTableTypeTo(table.first, m_tables); }
			void copyTypesTo(chunk* other) { other->copyTypesFrom(this); }

			size_t size() const { return m_ids.size(); }

			std::vector<id>& ids() { return m_ids; }
			const std::vector<id>& ids() const { return m_ids; }

			template<DataAccess accessType, typename ComponentsTuple> auto tables() {
				return getTables<accessType, ComponentsTuple>()(*this);
			}

			const dynamic_bitset& types() const { return m_types; }

			std::pair<migrate_move, migrate_move> migrateEntity(const dynamic_bitset& types, chunk* source, index_t source_index) {
				type_id_t typeId = types.nextOne(0);
				while (typeId != dynamic_bitset::npos) {
					source->m_tables.find(typeId)->second->moveFromIndexTo(source_index, m_tables.find(typeId)->second.get());
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

			template<typename T, typename U = std::remove_const_t<std::remove_reference_t<T>>> component_table<U> & table() {
				type_id_t typeIndex = m_typeIndex->id<U>();
				auto it = m_tables.find(typeIndex);
				if (it != m_tables.end())
					return *static_cast<component_table<U>*>(it->second.get());
				return *static_cast<component_table<U>*>(m_tables.emplace(typeIndex, std::make_unique<component_table<U>>()).first->second.get());
			}

			template<typename T> const auto& table() const { return const_cast<chunk*>(this)->table<T>(); }

		private:
			template<DataAccess, typename Ts> struct getTables {};
			template<DataAccess accessType, typename... Ts> struct getTables<accessType, std::tuple<Ts...>> {
				auto operator()(chunk& tableSource) {
					if constexpr (accessType == DataAccess::Const) {
						return std::make_tuple(&const_cast<const chunk&>(tableSource).table<Ts>()...);
					}
					else {
						return std::make_tuple(&tableSource.table<Ts>()...);
					}
				}
			};

			dynamic_bitset m_types;
			unordered_map<type_id_t, std::unique_ptr<itable>> m_tables;
			std::vector<id> m_ids;
			type_index* m_typeIndex;
		};


		template<DataAccess accessType, typename TableSource, typename F> class iterator {};
		template<DataAccess accessType, typename TableSource, typename Ret, typename Class, typename... Args>
		class iterator<accessType, TableSource, Ret(Class::*)(Args...) const> {
		public:
			iterator(TableSource& ecs) : m_ecs(ecs) { m_ecs.template componentTypesAllowed<Args...>(); }
			iterator(const TableSource& ecs) : m_ecs(const_cast<TableSource&>(ecs)) { m_ecs.template componentTypesAllowed<Args...>(); }

			template<typename F> void for_each(F&& op) {
				constexpr bool is_id_query = std::is_same_v<typename std::tuple_element<0, std::tuple<Args...> >::type, id>;

				// remove one type from front if we have id query.
				using components_tuple_type = typename remove_front_if<is_id_query, std::tuple<Args...>>::type;
				using components_index_seq_type = decltype(std::make_index_sequence<std::tuple_size<components_tuple_type>::value>());

				unpack_types<components_tuple_type>(components_index_seq_type());

				auto& chunks = m_ecs.chunks();
				for (auto&& chunk : chunks) {
					if (chunk.second->includesAll(includeTypes) && chunk.second->includesNone(excludeTypes)) {
						auto tables = chunk.second->template tables<accessType, components_tuple_type>();
						auto& ids = chunk.second->ids();
						for (index_t i = 0; i < ids.size(); ++i) {
							if constexpr (is_id_query) {
								make_indexed_call(std::forward<F>(op), i, ids[i].value, tables, components_index_seq_type());
							}
							else {
								make_indexed_call(std::forward<F>(op), i, tables, components_index_seq_type());
							}
						}
					}
				}
			}

			iterator& include(dynamic_bitset& inTypes) { includeTypes = inTypes; return *this; }
			iterator& exclude(dynamic_bitset& notInTypes) { excludeTypes = notInTypes; return *this; }

		private:
			template<typename F, typename... Ts, std::size_t... Is> void make_indexed_call(F&& op, index_t index, entity_id_t entityId, std::tuple<Ts...>& tuple, std::index_sequence<Is...>) { op(id(entityId), std::get<Is>(tuple)->operator[](index)...); }
			template<typename F, typename... Ts, std::size_t... Is> void make_indexed_call(F&& op, index_t index, std::tuple<Ts...>& tuple, std::index_sequence<Is...>) { op(std::get<Is>(tuple)->operator[](index)...); }

			template<typename TupleType, size_t...Is>
			void unpack_types(std::index_sequence<Is...>) { (includeTypes.set(m_ecs.template typeId<typename std::tuple_element<Is, TupleType>::type>()), ...); }

			dynamic_bitset includeTypes;
			dynamic_bitset excludeTypes;
			TableSource& m_ecs;
		};

		// TODO: don't use boolean template parameter. use enum class instead.
		template<typename TableSource, bool allowEditing = false>
		class entity {
		public:
			entity(TableSource& parent, entity_id_t id) : tableSource(&parent), m_id(id) {}

			template<typename T> T& get() {
				tableSource->template componentTypesAllowed<T>();
				auto chunkAndIndex = tableSource->chunk_and_index_for(m_id);
				rynx_assert(chunkAndIndex.first != nullptr, "referenced entity seems to not exist.");
				return chunkAndIndex.first->template table<T>()[chunkAndIndex.second];
			}

			template<typename... Ts> bool has() const {
				tableSource->template componentTypesAllowed<std::add_const_t<Ts> ...>();
				auto chunkAndIndex = tableSource->chunk_and_index_for(m_id);
				if (chunkAndIndex.first) {
					return true && (chunkAndIndex.first->types().test(tableSource->template typeId<Ts>()) && ...);
				}
				return false;
			}

			template<typename... Ts> void remove() {
				static_assert(allowEditing && sizeof...(Ts) >= 0, "You took this entity from an ecs::view. Removing components like this is not allowed. Use edit_view to remove components instead.");
				tableSource->template removeFromEntity<Ts...>(m_id);
			}
			template<typename T> void add(T&& t) {
				static_assert(allowEditing && sizeof(T) >= 0, "You took this entity from an ecs::view. Adding components like this is not allowed. Use edit_view to add components instead.");
				tableSource->template attachToEntity<T>(m_id, std::forward<T>(t));
			}
			entity_id_t id() const { return m_id; }
		private:
			TableSource* tableSource;
			entity_id_t m_id;
		};

		template<typename TableSource>
		class const_entity {
		public:
			const_entity(const TableSource& parent, entity_id_t id) : tableSource(&parent), m_id(id) {}
			template<typename T> const T& get() {
				tableSource->template componentTypesAllowed<T>();
				auto chunkAndIndex = tableSource->chunk_and_index_for(m_id);
				rynx_assert(chunkAndIndex.first != nullptr, "referenced entity seems to not exist.");
				return chunkAndIndex.first->template table<T>()[chunkAndIndex.second];
			}

			template<typename... Ts> bool has() const {
				tableSource->template componentTypesAllowed<Ts...>();
				auto chunkAndIndex = tableSource->chunk_and_index_for(m_id);
				if (chunkAndIndex.first) {
					return true && (chunkAndIndex.first->types().test(tableSource->template typeId<Ts>()) && ...);
				}
				return false;
			}

			template<typename... Ts> void remove() { static_assert(sizeof...(Ts) == 1 && sizeof...(Ts) == 2, "can't remove components from const entity"); }
			template<typename T, typename... Args> void add(Args&& ... args) const { static_assert(sizeof(T) == 1 && sizeof(T) == 2, "can't add components to const entity"); }
		private:
			const TableSource* tableSource;
			entity_id_t m_id;
		};

		template<DataAccess accessType, typename TableSource>
		class query_t {
			dynamic_bitset inTypes;
			dynamic_bitset notInTypes;
			TableSource& m_ecs;

		public:
			query_t(TableSource& ecs) : m_ecs(ecs) {}
			query_t(const TableSource& ecs_) : m_ecs(const_cast<TableSource&>(ecs_)) {}

			template<typename...Ts> query_t& in() { (inTypes.set(m_ecs.template typeId<Ts>()), ...); return *this; }
			template<typename...Ts> query_t& notIn() { (notInTypes.set(m_ecs.template typeId<Ts>()), ...); return *this; }

			template<typename F>
			query_t& execute(F&& op) {
				iterator<accessType, TableSource, decltype(&F::operator())> it(m_ecs);
				it.include(inTypes);
				it.exclude(notInTypes);
				it.for_each(std::forward<F>(op));
				return *this;
			}
		};

		ecs() = default;
		~ecs() {}

		bool exists(entity_id_t id) const { return m_idChunkMap.find(id) != m_idChunkMap.end(); }
		bool exists(id id) const { return exists(id.value); }

		entity<ecs, true> operator[](index_t id) { return entity<ecs, true>(*this, id); }
		entity<ecs, true> operator[](id id) { return entity<ecs, true>(*this, id.value); }
		const_entity<ecs> operator[](index_t id) const { return const_entity<ecs>(*this, id); }
		const_entity<ecs> operator[](id id) const { return const_entity<ecs>(*this, id.value); }

		template<typename F>
		void for_each(F&& op) {
			// PROFILE_SCOPE(Game, "ECS For Each");
			iterator<DataAccess::Mutable, ecs, decltype(&F::operator())> it(*this);
			it.for_each(std::forward<F>(op));
		}

		template<typename F>
		void for_each(F&& op) const {
			// PROFILE_SCOPE(Game, "ECS For Each");
			iterator<DataAccess::Const, ecs, decltype(&F::operator())> it(*this);
			it.for_each(std::forward<F>(op));
		}

		query_t<DataAccess::Mutable, ecs> query() { return query_t<DataAccess::Mutable, ecs>(*this); }
		query_t<DataAccess::Const, ecs> query() const { return query_t<DataAccess::Const, ecs>(*this); }

		std::pair<chunk*, index_t> chunk_and_index_for(entity_id_t id) const {
			auto it = m_idChunkMap.find(id);
			if (it != m_idChunkMap.end())
				return it->second;
			return std::pair<chunk*, index_t>(nullptr, 0);
		}

		void erase(entity_id_t entityId) {
			auto it = m_idChunkMap.find(entityId);
			if (it != m_idChunkMap.end()) {
				auto operations = it->second.first->erase(it->second.second);
				m_idChunkMap.erase(operations.first.id);
				if (operations.second.id != entity_index::InvalidId) {
					m_idChunkMap[operations.second.id] = { operations.second.newChunk, operations.second.newIndex };
				}
			}
			entities.entityKilled(entityId);
		}

		void erase(id entityId) { erase(entityId.value); }

		template<typename...Args> void componentTypesAllowed() const {}

		template<typename... Components>
		entity_id_t create(Components&& ... components) {
			entity_id_t id = entities.generateOne();
			dynamic_bitset targetChunk;
			(targetChunk.set(types.id<Components>()), ...);
			auto chunk_it = m_chunks.find(targetChunk);
			if (chunk_it == m_chunks.end()) { chunk_it = m_chunks.emplace(targetChunk, std::make_unique<chunk>(targetChunk, &types)).first; }
			chunk_it->second->insertNew(id, std::forward<Components>(components)...);
			m_idChunkMap.emplace(id, std::make_pair(chunk_it->second.get(), index_t(chunk_it->second->size() - 1)));
			return id;
		}

		template<typename... Components>
		ecs& attachToEntity(entity_id_t id, Components&& ... components) {
			auto it = m_idChunkMap.find(id);
			rynx_assert(it != m_idChunkMap.end(), "attachToEntity called for entity that does not exist.");
			const dynamic_bitset& initialTypes = it->second.first->types();
			dynamic_bitset resultTypes = initialTypes;
			(resultTypes.set(types.id<Components>()), ...);

			auto destinationChunkIt = m_chunks.find(resultTypes);
			if (destinationChunkIt == m_chunks.end()) {
				destinationChunkIt = m_chunks.emplace(resultTypes, std::make_unique<chunk>(resultTypes, &types)).first;
				destinationChunkIt->second->copyTypesFrom(it->second.first); // NOTE: Must make copies of table types for tables for which we don't know the type in this context.
			}

			// first migrate the old stuff, then apply on top of that what was given (in case these types overlap).
			auto changePair = destinationChunkIt->second->migrateEntity(initialTypes, it->second.first, it->second.second);
			//                                                          ^types moved, ^source chunk,    ^source index
			destinationChunkIt->second->insertComponents(std::forward<Components>(components)...);
			m_idChunkMap[changePair.first.id] = { changePair.first.newChunk, changePair.first.newIndex };
			if (changePair.second.newChunk)
				m_idChunkMap[changePair.second.id] = { changePair.second.newChunk, changePair.second.newIndex };

			return *this;
		}

		template<typename... Components>
		ecs& removeFromEntity(entity_id_t id) {
			auto it = m_idChunkMap.find(id);
			rynx_assert(it != m_idChunkMap.end(), "removeFromEntity called for entity that does not exist.");
			const dynamic_bitset& initialTypes = it->second.first->types();
			dynamic_bitset resultTypes = initialTypes;
			(resultTypes.reset(types.id<Components>()), ...);

			auto destinationChunkIt = m_chunks.find(resultTypes);
			if (destinationChunkIt == m_chunks.end()) {
				destinationChunkIt = m_chunks.emplace(resultTypes, std::make_unique<chunk>(resultTypes, &types)).first;
				destinationChunkIt->second->copyTypesFrom(it->second.first, resultTypes);
			}

			it->second.first->eraseComponentsFromIndex<Components...>(it->second.second);
			auto changePair = destinationChunkIt->second->migrateEntity(resultTypes, it->second.first, it->second.second);

			m_idChunkMap[changePair.first.id] = { changePair.first.newChunk, changePair.first.newIndex };
			if (changePair.second.newChunk)
				m_idChunkMap[changePair.second.id] = { changePair.second.newChunk, changePair.second.newIndex };
			return *this;
		}


		template<typename... Components> ecs& attachToEntity(id id, Components&& ... components) { attachToEntity(id.value, std::forward<Components>(components)...); return *this; }
		template<typename... Components> ecs& removeFromEntity(id id) { removeFromEntity<Components...>(id.value); return *this; }

	private:
		auto& chunks() { return m_chunks; }
		const auto& chunks() const { return m_chunks; }

	public:
		// ecs view that operates on Args types only. generates compile errors if trying to access any other data.
		// also does not allow creating new components, removing existing components or creating new entities.
		// if you need to do that, use edit_view instead.
		template<typename...TypeConstraints>
		class view {
			template<typename T, bool> friend class rynx::ecs::entity;
			template<DataAccess accessType, typename TableSource> friend class rynx::ecs::query_t;
			template<DataAccess accessType, typename TableSource, typename F> friend class rynx::ecs::iterator;

			template<typename T> static constexpr bool typeAllowed() { return isAny<std::remove_const_t<T>, id, std::remove_const_t<TypeConstraints>...>(); }
			template<typename T> static constexpr bool typeConstCorrect() {
				if constexpr (std::is_const_v<T>)
					return isAny<T, id, TypeConstraints...>() || isAny<std::remove_const_t<T>, id, TypeConstraints...>();
				else
					return isAny<T, id, TypeConstraints...>();
			}

			template<typename...Args> static constexpr bool typesAllowed() { return true && (typeAllowed<std::remove_reference_t<Args>>() && ...); }
			template<typename...Args> static constexpr bool typesConstCorrect() { return true && (typeConstCorrect<std::remove_reference_t<Args>>() && ...); }

		public:
			view(const rynx::ecs* ecs) : m_ecs(const_cast<rynx::ecs*>(ecs)) {}

			template<typename...Args> void componentTypesAllowed() const {
				static_assert(typesAllowed<Args...>(), "Your ecs view does not have access to one of requested types.");
				static_assert(typesConstCorrect<Args...>(), "You promised to access type in only const context.");
			}

			template<typename F>
			void for_each(F&& op) {
				// PROFILE_SCOPE(Game, "ECS For Each");
				iterator<DataAccess::Mutable, view, decltype(&F::operator())> it(*this);
				it.for_each(std::forward<F>(op));
			}

			template<typename F>
			void for_each(F&& op) const {
				// PROFILE_SCOPE(Game, "ECS For Each");
				iterator<DataAccess::Const, view, decltype(&F::operator())> it(*this);
				it.for_each(std::forward<F>(op));
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
			auto& chunks() { return m_ecs->chunks(); }
			const auto& chunks() const { return m_ecs->chunks(); }

			template<typename T> type_id_t typeId() const { return m_ecs->template typeId<T>(); }
			auto chunk_and_index_for(entity_id_t id) { return m_ecs->chunk_and_index_for(id); }

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

			template<typename... Components>
			entity_id_t create(Components&& ... components) {
				componentTypesAllowed<Components...>();
				return m_ecs->create(std::forward<Components>(components)...);
			}

			template<typename... Components>
			view& attachToEntity(entity_id_t id, Components&& ... components) {
				componentTypesAllowed<Components...>();
				m_ecs->attachToEntity(id, std::forward<Components>(components)...);
				return *this;
			}
			template<typename... Components> view& attachToEntity(id id, Components&& ... components) { return attachToEntity(id.value, std::forward<Components>(components)...); }

			template<typename... Components>
			view& removeFromEntity(entity_id_t id) {
				componentTypesAllowed<Components...>();
				m_ecs->removeFromEntity<Components...>(id);
				return *this;
			}
			template<typename... Components> view& removeFromEntity(id id, Components&& ... components) { return removeFromEntity<Components...>(id.value); }

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

		template<typename T> type_id_t typeId() const { return types.template id<T>(); }

		unordered_map<entity_id_t, std::pair<chunk*, index_t>> m_idChunkMap;
		unordered_map<dynamic_bitset, std::unique_ptr<chunk>, bitset_hash> m_chunks;
	};

}
