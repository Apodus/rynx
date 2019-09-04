#pragma once

#include <rynx/tech/dynamic_buffer.hpp>
#include <rynx/tech/dynamic_bitset.hpp>
#include <rynx/system/assert.hpp>

#include <utility>
#include <memory>
#include <functional> // for std::equal_to<T>

namespace rynx {

	template<typename T, typename U, typename Hash = std::hash<T>, class KeyEqual = std::equal_to<T>>
	class unordered_map {
		static constexpr uint32_t npos = ~uint32_t(0);

		// Array of structs layout
		/*
		inline uint32_t next_of_slot(size_t slot) const { return m_info[(slot << 2) + 0]; }
		inline uint32_t next_of_item(size_t slot) const { return m_info[(slot << 2) + 1]; }
		inline uint32_t prev_of_item(size_t slot) const { return m_info[(slot << 2) + 2]; }
		inline uint32_t hash_of_item(size_t slot) const { return m_info[(slot << 2) + 3]; }

		inline void update_next_of_slot(size_t slot, uint32_t value) { m_info[(slot << 2) + 0] = value; }
		inline void update_next_of_item(size_t slot, uint32_t value) { m_info[(slot << 2) + 1] = value; }
		inline void update_prev_of_item(size_t slot, uint32_t value) { m_info[(slot << 2) + 2] = value; }
		inline void update_hash_of_item(size_t slot, uint32_t value) { m_info[(slot << 2) + 3] = value; }
		*/

		// Structs of arrays layout
		inline uint32_t next_of_slot(size_t slot) const { return m_info[slot + 0 * m_capacity]; }
		inline uint32_t next_of_item(size_t slot) const { return m_info[slot + 1 * m_capacity]; }
		inline uint32_t prev_of_item(size_t slot) const { return m_info[slot + 2 * m_capacity]; }
		inline uint32_t hash_of_item(size_t slot) const { return m_info[slot + 3 * m_capacity]; }

		inline void update_next_of_slot(size_t slot, uint32_t value) { m_info[slot + 0 * m_capacity] = value; }
		inline void update_next_of_item(size_t slot, uint32_t value) { m_info[slot + 1 * m_capacity] = value; }
		inline void update_prev_of_item(size_t slot, uint32_t value) { m_info[slot + 2 * m_capacity] = value; }
		inline void update_hash_of_item(size_t slot, uint32_t value) { m_info[slot + 3 * m_capacity] = value; }

	public:
		using key_type = T;
		using mapped_type = U;
		using value_type = std::pair<const key_type, mapped_type>;
		using size_type = uint64_t;
		using difference_type = std::ptrdiff_t;
		using hasher = Hash;
		using key_equal = KeyEqual;
		using reference = value_type &;
		using const_reference = const value_type &;
		using pointer = value_type *;
		using const_pointer = const value_type*;

		class iterator {
		private:
			iterator(uint64_t index, dynamic_bitset* pPresence, std::aligned_storage_t<sizeof(std::pair<T, U>), alignof(std::pair<T, U>)>* pData) : m_index(index), m_pPresence(pPresence), m_pData(pData) {}
			friend class unordered_map;

		public:
			iterator(const iterator& other) = default;

			size_t index() const {
				return m_index;
			}

			iterator& operator ++() {
				m_index = m_pPresence->nextOne(m_index + 1);
				return *this;
			}

			iterator operator ++(int) {
				iterator copy(m_index, m_pPresence, m_pData);
				++(*this);
				return copy;
			}

			std::pair<T, U>& operator *() const { rynx_assert(m_pPresence->test(m_index), "invalid iterator dereference"); return *reinterpret_cast<std::pair<T, U>*>(m_pData + m_index); }
			std::pair<T, U>* operator ->() const { rynx_assert(m_pPresence->test(m_index), "invalid iterator dereference"); return reinterpret_cast<std::pair<T, U>*>(m_pData + m_index); }

			bool operator == (const iterator& other) const { return (m_index == other.m_index) && (m_pPresence == other.m_pPresence); }
			bool operator != (const iterator& other) const { return !((*this) == other); }

		private:
			uint64_t m_index;
			dynamic_bitset* m_pPresence;
			std::aligned_storage_t<sizeof(std::pair<T, U>), alignof(std::pair<T, U>)>* m_pData;
		};

		class const_iterator {
		private:
			const_iterator(uint64_t index, const dynamic_bitset* pPresence, const std::aligned_storage_t<sizeof(std::pair<T, U>), alignof(std::pair<T, U>)>* pData) : m_index(index), m_pPresence(pPresence), m_pData(pData) {}
			friend class unordered_map;

		public:
			const_iterator(const const_iterator& other) = default;
			const_iterator(const iterator& other) : m_index(other.m_index), m_pPresence(other.m_pPresence), m_pData(other.m_pData) {}

			size_t index() const {
				return m_index;
			}

			const_iterator& operator ++() {
				m_index = m_pPresence->nextOne(m_index + 1);
				return *this;
			}

			const_iterator operator ++(int) {
				const_iterator copy(m_index, m_pPresence, m_pData);
				++(*this);
				return copy;
			}

			const value_type& operator *() const { rynx_assert(m_pPresence->test(m_index), "invalid iterator dereference"); return *reinterpret_cast<const value_type*>(m_pData + m_index); }
			const value_type* operator ->() const { rynx_assert(m_pPresence->test(m_index), "invalid iterator dereference"); return reinterpret_cast<const value_type*>(m_pData + m_index); }

			bool operator == (const const_iterator& other) const { return (m_index == other.m_index) && (m_pPresence == other.m_pPresence); }
			bool operator != (const const_iterator& other) const { return !((*this) == other); }

		private:
			uint64_t m_index;
			const dynamic_bitset* m_pPresence;
			const std::aligned_storage_t<sizeof(std::pair<T, U>), alignof(std::pair<T, U>)>* m_pData;
		};

	private:


		inline value_type& item_in_slot(size_t slot) { rynx_assert(slot < m_capacity, "index out of bounds"); return *reinterpret_cast<value_type*>(m_data.get() + slot); }

		uint32_t try_pack_neighbors(uint32_t slot, uint32_t hash) {
			auto tryPackOne = [this](uint32_t slot, uint32_t hash) {
				auto distance_f = [this](uint32_t slot, uint32_t hash) { return slot > hash ? slot - hash : static_cast<uint32_t>(m_capacity) - hash + slot; };
				auto move_item_for_packing = [this](uint32_t from, uint32_t to) {
					new (m_data.get() + to) value_type(std::move(item_in_slot(from)));
					item_in_slot(from).~value_type();

					auto slot_of_from = hash_of_item(from);
					auto prev_of_from = prev_of_item(from);
					auto next_of_from = next_of_item(from);
					auto next_of_from_hash_slot = next_of_slot(slot_of_from);
					if (prev_of_from != npos) {
						update_next_of_item(prev_of_from, to);
						update_prev_of_item(to, prev_of_from);
					}
					if (next_of_from != npos) {
						update_prev_of_item(next_of_from, to);
						update_next_of_item(to, next_of_from);
					}
					if (next_of_from_hash_slot == from) {
						update_next_of_slot(slot_of_from, to);
					}

					update_prev_of_item(from, npos);
					update_next_of_item(from, npos);
					update_hash_of_item(from, npos);

					m_presence.reset(from);
					m_presence.set(to);
				};

				uint32_t current_distance = distance_f(slot, hash);
				uint32_t checkPos = (slot - 1) & (m_capacity - 1);
				uint32_t addedDistanceIfMoved = 1;
				while (checkPos != hash) {

					uint32_t dist_of_checkPos = distance_f(checkPos, hash_of_item(checkPos));
					if (dist_of_checkPos + addedDistanceIfMoved < current_distance) {
						move_item_for_packing(checkPos, slot);
						return checkPos;
					}

					++addedDistanceIfMoved;
					--checkPos;
					checkPos &= m_capacity - 1;
				}
				return slot;
			};

			for (;;) {
				uint32_t result = tryPackOne(slot, hash);
				if (result == slot)
					return slot;
				slot = result;
			}
		}

		// assumes: entry is not stored prior to insert.
		std::pair<iterator, bool> insert_unique_(value_type&& value, uint32_t hash) {
			if (m_capacity - m_size <= m_capacity >> 3) {
				if (m_capacity == 0)
					grow_to(64);
				else
					grow_to(m_capacity << 1);
				hash = static_cast<uint32_t>(Hash()(value.first) & (m_capacity - 1));
			}

			return unchecked_insert_(std::move(value), static_cast<uint32_t>(hash));
		}

		// this version bumps to back of bucket.
		// assumes: there is space for new entry.
		// assumes: entry is not stored prior to this insert.
		std::pair<iterator, bool> unchecked_insert_(value_type&& value, uint32_t hash) {
			++m_size;
			auto slot_forward = next_of_slot(hash);
			if (slot_forward == npos) {
				// find next available slot, and write it to next_of_slot.
				auto slot = m_presence.nextZero(hash);
				if (slot == dynamic_bitset::npos) {
					slot = m_presence.nextZero(0);
				}
				// slot = try_pack_neighbors(static_cast<uint32_t>(slot), hash);

				update_next_of_slot(hash, static_cast<uint32_t>(slot));
				update_hash_of_item(slot, static_cast<uint32_t>(hash));

				rynx_assert(!m_presence.test(slot), "inserting to reserved slot");
				new (m_data.get() + slot) value_type(std::move(value));
				m_presence.set(slot);

				return { iterator(slot, &m_presence, m_data.get()), true };
			}
			else {
				auto next_hash = slot_forward;
				auto item_forward = next_of_item(next_hash);
				while (item_forward != npos) {
					next_hash = item_forward;
					item_forward = next_of_item(next_hash);
				}

				// next_hash is now the last entry in current virtual bucket.
				// find an empty space and update next_of_item(next_hash) to point there.
				auto slot = m_presence.nextZero(next_hash + 1);
				if (slot == dynamic_bitset::npos) {
					slot = m_presence.nextZero(0);
				}
				// slot = try_pack_neighbors(static_cast<uint32_t>(slot), hash);

				update_next_of_item(next_hash, static_cast<uint32_t>(slot));
				update_prev_of_item(slot, static_cast<uint32_t>(next_hash));
				update_hash_of_item(slot, static_cast<uint32_t>(hash));

				rynx_assert(!m_presence.test(slot), "inserting to reserved slot");
				new (m_data.get() + slot) value_type(std::move(value));
				m_presence.set(slot);

				return { iterator(slot, &m_presence, m_data.get()), true };
			}
		}

		/*
		// this version bumps to front of bucket.
		std::pair<iterator, bool> unchecked_insert_(value_type&& value, uint32_t hash) {
			++m_size;

			auto place = m_presence.nextZero(hash);
			if (place == m_presence.npos)
				place = m_presence.nextZero(0);

			new (m_data.get() + place) value_type(std::move(value));
			m_presence.set(place);

			if (next_of_slot(hash) == npos) {
				update_next_of_slot(hash, static_cast<uint32_t>(place));
			}
			else {
				update_prev_of_item(next_of_slot(hash), static_cast<uint32_t>(place));
				update_next_of_item(place, next_of_slot(hash));
				update_next_of_slot(hash, static_cast<uint32_t>(place));
			}
			return { iterator(place, &m_presence, m_data.get()), true };
		}
		*/


		uint64_t find_index_(const T& key, size_t hash) const {
			if (empty())
				return dynamic_bitset::npos;

			auto first = next_of_slot(hash);
			for (;;) {
				if (first == npos)
					return dynamic_bitset::npos;
				if (KeyEqual()(reinterpret_cast<value_type*>(m_data.get() + first)->first, key))
					return first;
				first = next_of_item(first);
			}
		}

	public:
		unordered_map() {}
		unordered_map(const unordered_map& other) {
			for (const auto& entry : other)
				emplace(entry);
		}

		unordered_map(unordered_map&& other) {
			*this = std::move(other);
		}

		~unordered_map() {
			m_presence.forEachOne([this](uint64_t index) { reinterpret_cast<value_type*>(m_data.get() + index)->~value_type(); });
		}

		unordered_map& operator = (const unordered_map& other) {
			clear();
			for (const auto& entry : other)
				emplace(entry);
			return *this;
		}

		unordered_map& operator = (unordered_map&& other) noexcept {
			m_capacity = other.m_capacity;
			m_size = other.m_size;
			m_data = std::move(other.m_data);
			m_info = std::move(other.m_info);
			m_presence = std::move(other.m_presence);
			other.m_capacity = 0;
			other.m_size = 0;
			return *this;
		}

		iterator iteratorAt(size_t index) {
			if (index >= m_capacity)
				return end();
			return iterator(m_presence.nextOne(index), &m_presence, m_data.get());
		}

		const_iterator iteratorAt(uint64_t index) const { return const_cast<unordered_map*>(this)->iteratorAt(index); }

		/*
		TODO:
		node_type(since C++17)	a specialization of node handle representing a container node

		insert_return_type(since C++17)	type describing the result of inserting a node_type, a specialization of
		template <class Iter, class NodeType> struct unspecified {
			Iter     position;
			bool     inserted;
			NodeType node;
		};
		instantiated with template arguments iteratorand node_type.

		local_iterator	An iterator type whose category, value, difference, pointer and
		reference types are the same as iterator. This iterator
		can be used to iterate through a single bucket but not across buckets

		const_local_iterator


		todo:
		insert_return_type insert(node_type&& nh);
		iterator insert(const_iterator hint, node_type&& nh);

		node_type extract( const_iterator position );
		node_type extract( const key_type& x );

		template<class H2, class P2> void merge(std::unordered_map<Key, T, H2, P2, Allocator>& source);
		template<class H2, class P2> void merge(std::unordered_map<Key, T, H2, P2, Allocator>&& source);
		template<class H2, class P2> void merge(std::unordered_multimap<Key, T, H2, P2, Allocator>& source);
		template<class H2, class P2> void merge(std::unordered_multimap<Key, T, H2, P2, Allocator>&& source);

		equal_range functions not implemented todo.

		*/

		constexpr iterator begin() { return iterator(m_presence.nextOne(0), &m_presence, m_data.get()); }
		constexpr iterator end() { return iterator(dynamic_bitset::npos, &m_presence, m_data.get()); }
		constexpr const_iterator begin() const { return const_iterator(m_presence.nextOne(0), &m_presence, m_data.get()); }
		constexpr const_iterator end() const { return const_iterator(dynamic_bitset::npos, &m_presence, m_data.get()); }
		constexpr const_iterator cbegin() { return const_iterator(m_presence.nextOne(0), &m_presence, m_data.get()); }
		constexpr const_iterator cend() { return const_iterator(dynamic_bitset::npos, &m_presence, m_data.get()); }

		iterator find(const T& key) { return iterator(find_index_(key, Hash()(key) & (m_capacity - 1)), &m_presence, m_data.get()); }
		iterator find(const T& key, size_t hash) { return iterator(find_index_(key, hash & (m_capacity - 1)), &m_presence, m_data.get()); }
		const_iterator find(const T& key) const { return const_iterator(find_index_(key, Hash()(key) & (m_capacity - 1)), &m_presence, m_data.get()); }
		const_iterator find(const T& key, size_t hash) const { return const_iterator(find_index_(key, hash & (m_capacity - 1)), &m_presence, m_data.get()); }

		template<class K> iterator find(const K& x) { T t(x); return find(t); }
		template<class K> iterator find(const K& x, size_t hash) { T t(x); return find(t, hash); }
		template<class K> const_iterator find(const K& x) const { T t(x); return find(t); }
		template<class K> const_iterator find(const K& x, size_t hash) const { T t(x); return find(t, hash); }

		std::pair<iterator, bool> insert(const value_type& value) { return insert(value_type(value)); }
		std::pair<iterator, bool> insert(value_type&& value) {
			uint32_t hash = static_cast<uint32_t>(Hash()(value.first) & (m_capacity - 1));
			auto it = find(value.first, hash);
			if (it.m_index != dynamic_bitset::npos) {
				return { end(), false };
			}
			return insert_unique_(std::move(value), hash);
		}

		template<class P> std::pair<iterator, typename std::enable_if<std::is_constructible_v<value_type, P&&>, bool>::type> insert(P&& value) { return emplace(value_type(value)); }
		template<class P, typename std::enable_if<std::is_constructible_v<value_type, P&&>, void*>::type> iterator insert(const_iterator hint, P&& value) { return insert(std::forward<P>(value)).first; }

		iterator insert(const_iterator hint, const value_type& value) { return insert(value).first; }
		iterator insert(const_iterator hint, value_type&& value) { return insert(std::move(value)).first; }

		template<class InputIt> void insert(InputIt first, InputIt last) { while (first != last) insert(first++); }
		void insert(std::initializer_list<value_type> ilist) { for (auto&& entry : ilist) insert(std::move(entry)); }

		template <class M> iterator insert_or_assign(const_iterator, const key_type& k, M&& obj) { return insert_or_assign(k, U(std::forward<M>(obj))).first; }
		template <class M> iterator insert_or_assign(const_iterator, key_type&& k, M&& obj) { return insert_or_assign(std::move(k), U(std::forward<M>(obj))).first; }
		template <class M> std::pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj) {
			uint32_t hash = static_cast<uint32_t>(Hash()(k) & (m_capacity - 1));
			auto it = find(k, hash);
			if (it != end()) {
				it->second = std::forward<M>(obj);
				return { it, false };
			}
			else {
				return insert_unique_(
					value_type(
						k,
						std::forward<M>(obj)
					),
					hash
				);
			}
		}
		
		template <class M> std::pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj) {
			uint32_t hash = static_cast<uint32_t>(Hash()(k) & (m_capacity - 1));
			auto it = find(k, hash);
			if (it != end()) {
				it->second = std::forward<M>(obj);
				return { it, false };
			}
			else {
				return insert_unique_(
					value_type(
						std::move(k),
						std::forward<M>(obj)
					),
					hash
				);
			}
		}


		template <class... Args> std::pair<iterator, bool> try_emplace(const key_type& k, Args&& ... args) {
			uint32_t hash = static_cast<uint32_t>(Hash()(k) & (m_capacity - 1));
			auto it = find(k, hash);
			if (it != end()) {
				return { it, false };
			}
			else {
				return insert_unique_(
					value_type(
						k,
						mapped_type(std::forward<Args>(args)...)
					),
					hash
				);
			}
		}

		template <class... Args> std::pair<iterator, bool> try_emplace(key_type&& k, Args&& ... args) {
			uint32_t hash = static_cast<uint32_t>(Hash()(k) & (m_capacity - 1));
			auto it = find(k, hash);
			if (it != end()) {
				return { it, false };
			}
			else {
				return insert_unique_(
					value_type(
						std::move(k),
						mapped_type(std::forward<Args>(args)...)
					),
					hash
				);
			}
		}

		template <class... Args> iterator try_emplace(const_iterator, const key_type& k, Args&& ... args) { return try_emplace(k, std::forward<Args>(args)...).first; }
		template <class... Args> iterator try_emplace(const_iterator, key_type&& k, Args&& ... args) { return try_emplace(std::move(k), std::forward<Args>(args)...).first; }

		template<typename ... Args> std::pair<iterator, bool> emplace(Args&& ... args) { return insert(value_type(std::forward<Args>(args)...)); }
		template <class... Args> iterator emplace_hint(const_iterator, Args&& ... args) { return emplace(std::forward<Args>(args)...).first; }

		iterator erase(const_iterator pos) {
			erase_slot(pos.m_index);
			return iterator(m_presence.nextOne(pos.m_index), &m_presence, m_data.get());
		}

		iterator erase(const_iterator first, const_iterator last) {
			while (first != last) {
				erase(first);
				++first; // increment after erase is ok in this implementation.
			}
			return first;
		}

		float load_factor() const { if (m_size == 0) return 0;  return float(m_size) / float(m_capacity); }
		float max_load_factor() const { return 0.9f; } // scientific accuracy. trust me. I'm a doctor.

		void rehash(size_type count) { reserve(count); }
		void reserve(size_type count) {
			if (count < m_capacity)
				return;
			uint32_t requiredSize = 1;
			while (requiredSize < count)
				requiredSize <<= 1;
			grow_to(requiredSize);
		}

		size_type erase(const key_type& key) {
			auto it = find(key);
			if (it != end()) {
				erase(it);
				return 1;
			}
			return 0;
		}

		U& at(const T& key) {
			auto it = find(key);
			if (it != end())
				return it->second;
			throw std::out_of_range("key not found");
		}

		const U& at(const T& key) const { return const_cast<unordered_map*>(this)->at(key); }
		U& operator[](const T& key) { return try_emplace(key).first->second; }
		U& operator[](T&& key) { return try_emplace(std::move(key)).first->second; }

		size_type count(const T& key) { return find(key) != end() ? 1 : 0; }
		size_type count(const T& key, std::size_t hash) { return find(key, hash) != end() ? 1 : 0; }
		// template<class K> size_type count(const K& x) const; (since C++20)
		// template<class K> size_type count(const K& x, std::size_t hash) const; (since C++20)

		bool contains(const T& key) const { return find(key).m_index != dynamic_bitset::npos; }
		bool contains(const T& key, std::size_t hash) { return find(key, hash).m_index != dynamic_bitset::npos; }
		template<class K> bool contains(const K& x) const { return find(x).m_index != dynamic_bitset::npos; }
		template<class K> bool contains(const K& x, std::size_t hash) const { return find(x, hash).m_index != dynamic_bitset::npos; }

		void swap(unordered_map& other) noexcept {
			std::swap(m_capacity, other.m_capacity);
			std::swap(m_size, other.m_size);
			std::swap(m_presence, other.m_presence);
			std::swap(m_info, other.m_info);
			std::swap(m_data, other.m_data);
		}

		constexpr bool empty() const { return m_size == 0; }
		constexpr size_t size() const { return m_size; }
		constexpr size_t max_size() const { return ~uint32_t(0); }
		Hash hash_function() const { return Hash(); }
		KeyEqual key_eq() const { return KeyEqual(); }
		void clear() noexcept {
			auto it_end = end();
			for (auto it = begin(); it != it_end; ++it) {
				erase(it);
			}
		}

	private:
		void erase_slot(size_t slot_index) {
			if (next_of_item(slot_index) != npos) {
				update_prev_of_item(next_of_item(slot_index), prev_of_item(slot_index));
			}

			if (prev_of_item(slot_index) != npos) {
				update_next_of_item(prev_of_item(slot_index), next_of_item(slot_index));
			}
			else {
				update_next_of_slot(hash_of_item(slot_index), next_of_item(slot_index));
			}

			// todo: these should be done with one call.
			update_next_of_item(slot_index, npos);
			update_prev_of_item(slot_index, npos);
			update_hash_of_item(slot_index, npos);

			// call desctructor
			item_in_slot(slot_index).~value_type();

			// remove from presence bitset
			m_presence.reset(slot_index);
			--m_size;
		}

		void reserve_memory_internal(size_t s) {
			m_data.reset(new std::aligned_storage_t<sizeof(value_type), alignof(value_type)>[s]);
			m_info.resize_discard(4 * s, ~uint8_t(0));
			m_presence.resize_bits(s);
			m_capacity = s;
		}

		void grow_to(size_t s) {
			unordered_map other;
			other.reserve_memory_internal(s);

			for (auto&& pair : *this) {
				other.unchecked_insert_(std::move(pair), static_cast<uint32_t>(Hash()(pair.first) & (other.m_capacity - 1)));
			}

			*this = std::move(other);
		}

		std::unique_ptr<std::aligned_storage_t<sizeof(value_type), alignof(value_type)>[]> m_data;
		dynamic_buffer<uint32_t> m_info;
		dynamic_bitset m_presence;

		size_t m_capacity = 0;
		size_t m_size = 0;
	};
}