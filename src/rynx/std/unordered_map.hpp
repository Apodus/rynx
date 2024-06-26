#pragma once

#include <rynx/std/dynamic_buffer.hpp>
#include <rynx/std/dynamic_bitset.hpp>
#include <rynx/system/assert.hpp>
#include <rynx/std/memory.hpp>
#include <type_traits>

namespace rynx {
	template<typename T, typename U>
	struct pair {
		using first_type = T;
		using second_type = U;

		template <class _Uty1 = T, class _Uty2 = U,
			std::enable_if_t<std::conjunction_v<std::is_default_constructible<_Uty1>, std::is_default_constructible<_Uty2>>, int> = 0>
		constexpr pair() noexcept(std::is_nothrow_default_constructible_v<_Uty1> && std::is_nothrow_default_constructible_v<_Uty2>)
			: first(), second() {}

		template <class _Uty1 = T, class _Uty2 = U,
			std::enable_if_t<std::conjunction_v<std::is_copy_constructible<_Uty1>, std::is_copy_constructible<_Uty2>>, int> = 0>
		constexpr explicit(!std::conjunction_v<std::is_convertible<const _Uty1&, _Uty1>, std::is_convertible<const _Uty2&, _Uty2>>)
			pair(const T& _Val1, const U& _Val2) noexcept(std::is_nothrow_copy_constructible_v<_Uty1> && std::is_nothrow_copy_constructible_v<_Uty2>)
			: first(_Val1), second(_Val2) {}

		template <class _Other1 = T, class _Other2 = U,
			std::enable_if_t<std::conjunction_v<std::is_constructible<T, _Other1>, std::is_constructible<U, _Other2>>, int> = 0>
		constexpr explicit(!std::conjunction_v<std::is_convertible<_Other1, T>, std::is_convertible<_Other2, U>>)
			pair(_Other1&& _Val1, _Other2&& _Val2) noexcept(
				std::is_nothrow_constructible_v<T, _Other1>&& std::is_nothrow_constructible_v<U, _Other2>) // strengthened
			: first(std::forward<_Other1>(_Val1)), second(std::forward<_Other2>(_Val2)) {}

		pair(const pair&) = default;
		pair(pair&&) = default;

		template <class _Other1, class _Other2,
			std::enable_if_t<std::conjunction_v<std::is_constructible<T, const _Other1&>, std::is_constructible<U, const _Other2&>>, int> = 0>
		constexpr explicit(!std::conjunction_v<std::is_convertible<const _Other1&, T>, std::is_convertible<const _Other2&, U>>)
			pair(const pair<_Other1, _Other2>& _Right) noexcept(std::is_nothrow_constructible_v<T, const _Other1&> && std::is_nothrow_constructible_v<U, const _Other2&>)
			: first(_Right.first), second(_Right.second) {}

		template <class _Other1, class _Other2,
			std::enable_if_t<std::conjunction_v<std::is_constructible<T, _Other1>, std::is_constructible<U, _Other2>>, int> = 0>
		constexpr explicit(!std::conjunction_v<std::is_convertible<_Other1, T>, std::is_convertible<_Other2, U>>)
			pair(pair<_Other1, _Other2>&& _Right) noexcept(
				std::is_nothrow_constructible_v<T, _Other1> && std::is_nothrow_constructible_v<U, _Other2>)
			: first(std::forward<_Other1>(_Right.first)), second(std::forward<_Other2>(_Right.second)) {}

		pair& operator=(const volatile pair&) = delete;

		template <class Other1, class Other2>
			constexpr pair& operator=(const pair<Other1, Other2>& other) noexcept (std::is_nothrow_assignable_v<T&, const Other1&> && std::is_nothrow_assignable_v<U&, const Other2&>) {
			first = other.first;
			second = other.second;
			return *this;
		}

		template <class Other1, class Other2>
			constexpr pair& operator=(pair<Other1, Other2>&& other) noexcept(std::is_nothrow_assignable_v<T&, Other1> && std::is_nothrow_assignable_v<U&, Other2>) {
			first = std::forward<Other1>(other.first);
			second = std::forward<Other2>(other.second);
			return *this;
		}

		constexpr void swap(pair& other) {
			if (this != &other) {
				std::swap(first, other.first);
				std::swap(second, other.second);
			}
		}
		
		T first;
		U second;
	};
	
	template <class _Ty1, class _Ty2>
	pair(_Ty1, _Ty2) -> pair<_Ty1, _Ty2>;

	template<typename T>
    struct equal_to
    {
      constexpr bool operator()(const T& x, const T& y) const { return x == y; }
    };

	template<typename T, typename U, typename Hash = std::hash<T>, class KeyEqual = rynx::equal_to<T>>
	class unordered_map {
#if 0
		// Array of structs layout
		inline uint32_t next_of_slot(size_t slot) const { return m_info[(slot << 2) + 0]; }
		inline uint32_t next_of_item(size_t slot) const { return m_info[(slot << 2) + 1]; }
		inline uint32_t prev_of_item(size_t slot) const { return m_info[(slot << 2) + 2]; }
		inline uint32_t hash_of_item(size_t slot) const { return m_info[(slot << 2) + 3]; }

		inline void update_next_of_slot(size_t slot, uint32_t value) { m_info[(slot << 2) + 0] = value; }
		inline void update_next_of_item(size_t slot, uint32_t value) { m_info[(slot << 2) + 1] = value; }
		inline void update_prev_of_item(size_t slot, uint32_t value) { m_info[(slot << 2) + 2] = value; }
		inline void update_hash_of_item(size_t slot, uint32_t value) { m_info[(slot << 2) + 3] = value; }
#else	
		// Structs of arrays layout. Better for find.
		inline uint32_t next_of_slot(size_t slot) const noexcept { return m_info[slot + 0 * static_cast<uint64_t>(m_capacity)]; }
		inline uint32_t next_of_item(size_t slot) const noexcept { return m_info[slot + 1 * static_cast<uint64_t>(m_capacity)]; }
		inline uint32_t prev_of_item(size_t slot) const noexcept { return m_info[slot + 2 * static_cast<uint64_t>(m_capacity)]; }
		inline uint32_t hash_of_item(size_t slot) const noexcept { return m_info[slot + 3 * static_cast<uint64_t>(m_capacity)]; }

		inline void update_next_of_slot(size_t slot, uint32_t value) noexcept { m_info[slot + 0 * static_cast<uint64_t>(m_capacity)] = value; }
		inline void update_next_of_item(size_t slot, uint32_t value) noexcept { m_info[slot + 1 * static_cast<uint64_t>(m_capacity)] = value; }
		inline void update_prev_of_item(size_t slot, uint32_t value) noexcept { m_info[slot + 2 * static_cast<uint64_t>(m_capacity)] = value; }
		inline void update_hash_of_item(size_t slot, uint32_t value) noexcept { m_info[slot + 3 * static_cast<uint64_t>(m_capacity)] = value; }
#endif

	public:
		using key_type = T;
		using mapped_type = U;
		using value_type = pair<const key_type, mapped_type>;
		using size_type = uint64_t;
		using difference_type = std::ptrdiff_t;
		using hasher = Hash;
		using key_equal = KeyEqual;
		using reference = value_type &;
		using const_reference = const value_type &;
		using pointer = value_type *;
		using const_pointer = const value_type*;

		class storage_t {
		private:
			alignas(value_type) std::byte buffer[sizeof(value_type)];
		};

		class iterator {
		private:
			iterator(uint64_t index, dynamic_bitset* pPresence, storage_t* pData) : m_index(index), m_pPresence(pPresence), m_pData(pData) {}
			friend class unordered_map;

		public:
			iterator(const iterator& other) = default;

			size_t index() const noexcept {
				return m_index;
			}

			iterator& operator ++() noexcept {
				m_index = m_pPresence->nextOne(m_index + 1);
				return *this;
			}

			iterator operator ++(int) {
				iterator copy(m_index, m_pPresence, m_pData);
				++(*this);
				return copy;
			}

			pair<T, U>& operator *() const { rynx_assert(m_pPresence->test(m_index), "invalid iterator dereference"); return *reinterpret_cast<pair<T, U>*>(m_pData + m_index); }
			pair<T, U>* operator ->() const { rynx_assert(m_pPresence->test(m_index), "invalid iterator dereference"); return reinterpret_cast<pair<T, U>*>(m_pData + m_index); }

			bool operator == (const iterator& other) const { rynx_assert(m_pPresence == other.m_pPresence, "comparing iterators between different container instances"); return (m_index == other.m_index); }
			bool operator != (const iterator& other) const { return !((*this) == other); }

		private:
			uint64_t m_index;
			dynamic_bitset* m_pPresence;
			storage_t* m_pData;
		};

		class const_iterator {
		private:
			const_iterator(uint64_t index, const dynamic_bitset* pPresence, const storage_t* pData) : m_index(index), m_pPresence(pPresence), m_pData(pData) {}
			friend class unordered_map;

		public:
			const_iterator(const const_iterator& other) = default;
			const_iterator(const iterator& other) : m_index(other.m_index), m_pPresence(other.m_pPresence), m_pData(other.m_pData) {}

			size_t index() const noexcept {
				return m_index;
			}

			const_iterator& operator ++() noexcept {
				m_index = m_pPresence->nextOne(m_index + 1);
				return *this;
			}

			const_iterator operator ++(int) {
				const_iterator copy(m_index, m_pPresence, m_pData);
				++(*this);
				return copy;
			}

			const value_type& operator *() const noexcept {
				rynx_assert(m_pPresence->test(m_index), "invalid iterator dereference");
				return *reinterpret_cast<const value_type*>(m_pData + m_index);
			}
			
			const value_type* operator ->() const noexcept {
				rynx_assert(m_pPresence->test(m_index), "invalid iterator dereference");
				return reinterpret_cast<const value_type*>(m_pData + m_index);
			}

			bool operator == (const const_iterator& other) const { rynx_assert(m_pPresence == other.m_pPresence, "comparing iterators between different container instances"); return (m_index == other.m_index); }
			bool operator != (const const_iterator& other) const { return !((*this) == other); }

		private:
			uint64_t m_index;
			const dynamic_bitset* m_pPresence;
			const storage_t* m_pData;
		};

	private:


		inline value_type& item_in_slot(size_t slot) { rynx_assert(slot < m_capacity, "index out of bounds"); return *reinterpret_cast<value_type*>(m_data.get() + slot); }

		// assumes: entry is not stored prior to insert.
		pair<iterator, bool> insert_unique_(value_type&& value, uint32_t hash) {
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
		pair<iterator, bool> unchecked_insert_(value_type&& value, uint32_t hash) {
			++m_size;
			auto slot_forward = next_of_slot(hash);
			if (slot_forward == m_capacity) {
				// find next available slot, and write it to next_of_slot.
				auto slot = m_presence.nextZero(hash);
				if (slot == dynamic_bitset::npos) {
					slot = m_presence.nextZero(0);
				}
				
				rynx_assert(!m_presence.test(slot), "inserting to reserved slot");
				new (m_data.get() + slot) value_type(std::move(value));
				m_presence.set(slot);

				update_next_of_slot(hash, static_cast<uint32_t>(slot));
				update_hash_of_item(slot, static_cast<uint32_t>(hash));

				return { iterator(slot, &m_presence, m_data.get()), true };
			}
			else {
				auto next_hash = slot_forward;
				auto item_forward = next_of_item(next_hash);
				while (item_forward != m_capacity) {
					next_hash = item_forward;
					item_forward = next_of_item(next_hash);
				}

				// next_hash is now the last entry in current virtual bucket.
				// find an empty space and update next_of_item(next_hash) to point there.
				auto slot = m_presence.nextZero(next_hash + 1);
				if (slot == dynamic_bitset::npos) {
					slot = m_presence.nextZero(0);
				}
				
				rynx_assert(!m_presence.test(slot), "inserting to reserved slot");
				new (m_data.get() + slot) value_type(std::move(value));
				m_presence.set(slot);

				update_next_of_item(next_hash, static_cast<uint32_t>(slot));
				update_prev_of_item(slot, static_cast<uint32_t>(next_hash));
				update_hash_of_item(slot, static_cast<uint32_t>(hash));

				return { iterator(slot, &m_presence, m_data.get()), true };
			}
		}

		/*
		// this version bumps to front of bucket.
		pair<iterator, bool> unchecked_insert_(value_type&& value, uint32_t hash) {
			++m_size;

			auto place = m_presence.nextZero(hash);
			if (place == m_presence.npos)
				place = m_presence.nextZero(0);

			new (m_data.get() + place) value_type(std::move(value));
			m_presence.set(place);

			update_hash_of_item(place, static_cast<uint32_t>(hash));
			if (next_of_slot(hash) == m_capacity) {
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

		uint64_t find_index_(const T& key, size_t hash) const noexcept {
			//if (empty())
			//	return dynamic_bitset::npos;

			auto first = next_of_slot(hash);
			for (;;) {
				if (first == m_capacity)
					return dynamic_bitset::npos;
				if (KeyEqual()(reinterpret_cast<value_type*>(m_data.get() + first)->first, key))
					return first;
				first = next_of_item(first);
			}
		}

	public:
		unordered_map() {
			reserve_memory_internal(64);
		}
		
		template<typename EnableType = std::enable_if_t< std::is_copy_constructible_v<U> > >
		unordered_map(const unordered_map& other) {
			reserve_memory_internal(other.capacity());
			for (auto it = other.begin(); it != other.end(); ++it) {
				unchecked_insert_(it->second, it.m_index);
			}
		}

		unordered_map(unordered_map&& other) {
			*this = std::move(other);
		}

		~unordered_map() {
			m_presence.forEachOne([this](uint64_t index) { reinterpret_cast<value_type*>(m_data.get() + index)->~value_type(); });
		}

		unordered_map& operator = (const unordered_map& other) {
			if constexpr (std::is_copy_assignable_v<T> && std::is_copy_assignable_v<U>) {
				clear();
				for (const auto& entry : other)
					emplace(entry);
				return *this;
			}
			else {
				rynx_assert(false, "todo fix");
				return *this;
			}
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

		iterator iterator_at(size_t index) {
			if (index >= m_capacity)
				return end();
			return iterator(m_presence.nextOne(index), &m_presence, m_data.get());
		}

		const_iterator iterator_at(uint64_t index) const { return const_cast<unordered_map*>(this)->iterator_at(index); }

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

		pair<iterator, bool> insert(const value_type& value) { return insert(value_type(value)); }
		pair<iterator, bool> insert(value_type&& value) {
			uint32_t hash = static_cast<uint32_t>(Hash()(value.first) & (m_capacity - 1));
			auto it = find(value.first, hash);
			if (it.m_index != dynamic_bitset::npos) {
				return { it, false };
			}
			return insert_unique_(std::move(value), hash);
		}

		template<class P> pair<iterator, typename std::enable_if<std::is_constructible_v<value_type, P&&>, bool>::type> insert(P&& value) { return emplace(value_type(value)); }
		template<class P, typename std::enable_if<std::is_constructible_v<value_type, P&&>, void*>::type> iterator insert(const_iterator hint, P&& value) { return insert(std::forward<P>(value)).first; }

		iterator insert(const_iterator hint, const value_type& value) { return insert(value).first; }
		iterator insert(const_iterator hint, value_type&& value) { return insert(std::move(value)).first; }

		template<class InputIt> void insert(InputIt first, InputIt last) { while (first != last) insert(first++); }
		void insert(std::initializer_list<value_type> ilist) { for (auto&& entry : ilist) insert(std::move(entry)); }

		template <class M> iterator insert_or_assign(const_iterator, const key_type& k, M&& obj) { return insert_or_assign(k, U(std::forward<M>(obj))).first; }
		template <class M> iterator insert_or_assign(const_iterator, key_type&& k, M&& obj) { return insert_or_assign(std::move(k), U(std::forward<M>(obj))).first; }
		template <class M> pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj) {
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
		
		template <class M> pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj) {
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


		template <class... Args> pair<iterator, bool> try_emplace(const key_type& k, Args&& ... args) {
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

		template <class... Args> pair<iterator, bool> try_emplace(key_type&& k, Args&& ... args) {
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

		template<typename ... Args> pair<iterator, bool> emplace(Args&& ... args) { return insert(value_type(std::forward<Args>(args)...)); }
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
			rynx_assert(false, "key not found");
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

		constexpr bool empty() const noexcept { return m_size == 0; }
		constexpr size_t size() const noexcept { return m_size; }
		constexpr size_t capacity() const noexcept { return m_capacity; }
		
		// slot_* functions are very much non-standard. but useful in parallel_for.
		bool slot_test(int64_t index) const noexcept { return m_presence.test(index); }
		const value_type& slot_get(int64_t index) const noexcept { return *reinterpret_cast<const value_type*>(&m_data.get()[index]); }

		constexpr size_t max_size() const noexcept { return ~uint32_t(0); }
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
			if (next_of_item(slot_index) != m_capacity) {
				update_prev_of_item(next_of_item(slot_index), prev_of_item(slot_index));
			}

			if (prev_of_item(slot_index) != m_capacity) {
				update_next_of_item(prev_of_item(slot_index), next_of_item(slot_index));
			}
			else {
				update_next_of_slot(hash_of_item(slot_index), next_of_item(slot_index));
			}

			// todo: these should be done with one call.
			update_next_of_item(slot_index, m_capacity);
			update_prev_of_item(slot_index, m_capacity);
			update_hash_of_item(slot_index, m_capacity);

			// call desctructor
			item_in_slot(slot_index).~value_type();

			// remove from presence bitset
			m_presence.reset(slot_index);
			--m_size;
		}

		void reserve_memory_internal(size_t s) {
			m_data.reset(new storage_t[s + 1]); // +1 for end value.
			m_info.resize_discard(4 * s, static_cast<uint32_t>(s));
			m_presence.resize_bits(s);
			rynx_assert(s < (1llu << 32), "unordered_map does not supports sizes greater than 2^32");
			m_capacity = static_cast<uint32_t>(s);
			new (m_data.get() + m_capacity) value_type();
		}

		void grow_to(uint32_t s) {
			unordered_map other;
			other.reserve_memory_internal(s);

			for (auto&& pair : *this) {
				other.unchecked_insert_(std::move(pair), static_cast<uint32_t>(Hash()(pair.first) & (other.m_capacity - 1)));
			}

			*this = std::move(other);
		}

		rynx::unique_ptr<storage_t[]> m_data;
		dynamic_buffer<uint32_t> m_info;
		dynamic_bitset m_presence;

		uint32_t m_capacity = 0;
		uint32_t m_size = 0;
	};
}


#include <rynx/std/serialization_declares.hpp>

namespace rynx {
	namespace serialization {
		template<typename T, typename U> struct Serialize<rynx::unordered_map<T, U>> {
			template<typename IOStream>
			void serialize(const rynx::unordered_map<T, U>& map_t, IOStream& writer) {
				writer(map_t.size());
				for (auto&& t : map_t) {
					rynx::serialize(t.first, writer);
					rynx::serialize(t.second, writer);
				}
			}

			template<typename IOStream>
			void deserialize(rynx::unordered_map<T, U>& map, IOStream& reader) {
				size_t numElements = rynx::deserialize<size_t>(reader);
				for (size_t i = 0; i < numElements; ++i) {
					T t = rynx::deserialize<T>(reader);
					U u = rynx::deserialize<U>(reader);
					map.emplace(std::move(t), std::move(u));
				}
			}
		};
	}
}