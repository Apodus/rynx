
#pragma once

#include <rynx/std/serialization.hpp>
#include <rynx/ecs/id.hpp>
#include <rynx/std/memory.hpp>
#include <rynx/system/assert.hpp>
#include <rynx/system/typeid.hpp>
#include <vector>
#include <numeric>

namespace rynx {
	namespace ecs_internal {
		class itable {
		public:
			itable(uint64_t type_id) : m_type_id(type_id) {}
			virtual ~itable() {}
			virtual rynx::unique_ptr<itable> clone_ptr() const = 0;
			
			virtual void erase(entity_id_t entityId) = 0;
			virtual void insert(opaque_unique_ptr<void> data) = 0;
			virtual bool equals(index_t index, const void* data) const = 0;

			virtual void serialize(rynx::serialization::vector_writer& writer) = 0;
			virtual void deserialize(rynx::serialization::vector_reader& reader) = 0;
			
			virtual void serialize_index(rynx::serialization::vector_writer& writer, index_t entity_index) = 0;
			virtual void deserialize_index(rynx::serialization::vector_reader& reader, index_t entity_index) = 0;

			virtual void for_each_id_field(rynx::function<void(rynx::id&)>) = 0;

			// serialization, only top-most level scene entities should be touched. and their id links need to be transformed
			// to persistent scene id chains.
			virtual void for_each_id_field_for_single_index(index_t index, rynx::function<void(rynx::id&)> op) = 0;
			
			// when we already have content in ecs, and we deserialize (load) more content in,
			// we need to operate over all deserialized entities but to not touch pre-existing data.
			virtual void for_each_id_field_from_index(size_t startingIndex, rynx::function<void(rynx::id&)>) = 0;

			virtual void swap_adjacent_indices_for(const std::vector<index_t>& index_points) = 0;
			virtual void swap_to_given_order(const std::vector<index_t>& relative_sort_order) = 0;

			virtual void replace_with_back_and_pop(index_t i) = 0;

			virtual void* get(index_t) = 0;
			virtual const void* get(index_t) const = 0;
			virtual rynx::string type_name() const = 0;
			virtual bool is_type_segregated() const = 0;

			virtual void copyTableTypeTo(type_id_t typeId, std::vector<rynx::unique_ptr<itable>>& targetTables) = 0;
			virtual void moveFromIndexTo(index_t index, itable* dst) = 0;

			uint64_t m_type_id;
		};

		template<typename T>
		class component_table : public itable {
		public:
			component_table(uint64_t type_id) : itable(type_id) {}
			component_table(const component_table& other) : itable(other.m_type_id) {
				m_data = other.m_data; // to support cloning.
			}

			virtual ~component_table() {}

			virtual rynx::unique_ptr<itable> clone_ptr() const override {
				return rynx::make_unique<component_table<T>>(component_table<T>(*this));
			}

			virtual void insert(opaque_unique_ptr<void> data) override {
				m_data.emplace_back(std::move(*static_cast<T*>(data.get())));
			}

			// used for serializing diffs only.
			virtual bool equals(index_t index, const void* data) const override {
				return rynx::component_equals(m_data[index], *static_cast<const T*>(data));
			}

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

			virtual void serialize(rynx::serialization::vector_writer& writer) override {
				if constexpr (!std::is_base_of_v<ecs_no_serialize_tag, T>)
					rynx::serialize(m_data, writer);
			}

			virtual void deserialize(rynx::serialization::vector_reader& reader) override {
				rynx::deserialize(m_data, reader);
			}

			virtual void serialize_index(rynx::serialization::vector_writer & writer, index_t entity_index) override
			{
				if constexpr (!std::is_base_of_v<ecs_no_serialize_tag, T>)
					rynx::serialize(m_data[entity_index], writer);
			}

			virtual void deserialize_index(rynx::serialization::vector_reader& reader, index_t entity_index) override
			{
				rynx::deserialize(m_data[entity_index], reader);
			}

			// TODO: skip iterating over data if no types in hierarchy have id members.
			virtual void for_each_id_field(rynx::function<void(rynx::id&)> op) override {
				for (auto& entry : m_data) {
					rynx::for_each_id_field(entry, op);
				}
			}

			virtual void for_each_id_field_for_single_index(index_t index, rynx::function<void(rynx::id&)> op) override {
				rynx::for_each_id_field(m_data[index], op);
			}

			// TODO: skip iterating over data if no types in hierarchy have id members.
			virtual void for_each_id_field_from_index(size_t startingIndex, rynx::function<void(rynx::id&)> op) override {
				for (size_t i = startingIndex; i < m_data.size(); ++i) {
					rynx::for_each_id_field(m_data[i], op);
				}
			}

			virtual void replace_with_back_and_pop(index_t i) override {
				m_data[i] = std::move(m_data.back());
				m_data.pop_back();
			}

			virtual void* get(index_t i) override { return &m_data[i]; }
			virtual const void* get(index_t i) const override { return &m_data[i]; }
			
			virtual rynx::string type_name() const override {
				return rynx::traits::template type_name<T>();
			}
			virtual bool is_type_segregated() const override {
				return std::is_base_of_v<rynx::ecs_value_segregated_component_tag, T>;
			}

			virtual void erase(entity_id_t id) override {
				m_data[id] = std::move(m_data.back());
				m_data.pop_back();
			}

			// NOTE: This is required for moving entities between categories reliably.
			virtual void copyTableTypeTo(type_id_t typeId, std::vector<rynx::unique_ptr<itable>>& targetTables) override {
				if (typeId >= targetTables.size()) {
					targetTables.resize(((3 * typeId) >> 1) + 1);
				}
				targetTables[typeId] = rynx::make_unique<component_table>(typeId);
			}

			virtual void moveFromIndexTo(index_t index, itable* dst) override {
				static_cast<component_table*>(dst)->emplace_back(std::move(m_data[index]));
				m_data[index] = std::move(m_data.back());
				m_data.pop_back();
			}

			void insert(T&& t) { m_data.emplace_back(std::forward<T>(t)); }
			void insert(std::vector<T>& v) { m_data.insert(m_data.end(), v.begin(), v.end()); }
			template<typename...Ts> void emplace_back(Ts&& ... ts) { m_data.emplace_back(std::forward<Ts>(ts)...); }

			void insert_default(size_t n) {
				m_data.resize(m_data.size() + n);
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

		// used for value hashes in value segregated component types.
		template<typename T>
		struct local_hash_function {
			size_t operator()(const T& t) {
				return t.hash();
			}
		};

		struct ivalue_segregation_map {
			virtual ~ivalue_segregation_map() {}
			virtual std::vector<rynx::type_index::virtual_type> get_virtual_types() const = 0;
			virtual bool contains(void*) = 0;
			virtual void emplace(void* data, rynx::type_index::virtual_type type) = 0;
			virtual rynx::type_index::virtual_type get_type_id_for(void* data) = 0;
			virtual ivalue_segregation_map* clone_ptr() const = 0;
		};

		template<typename T>
		struct value_segregation_map : public ivalue_segregation_map {
			virtual std::vector<rynx::type_index::virtual_type> get_virtual_types() const override {
				std::vector<rynx::type_index::virtual_type> result;
				for (auto&& entry : m_map)
					result.emplace_back(entry.second);
				return result;
			}
			
			virtual bool contains(void* data) override {
				return m_map.find(*static_cast<T*>(data)) != m_map.end();
			}

			virtual void emplace(void* data, rynx::type_index::virtual_type type) {
				m_map.emplace(*static_cast<T*>(data), type);
			}

			virtual rynx::type_index::virtual_type get_type_id_for(void* data) {
				return m_map.find(*static_cast<T*>(data))->second;
			}

			virtual ivalue_segregation_map* clone_ptr() const {
				auto* ptr = new value_segregation_map();
				ptr->m_map = m_map;
				return ptr;
			}

			void emplace(const T& t, rynx::type_index::virtual_type type) {
				m_map.emplace(t, type);
			}

			auto find(const T& t) const {
				return m_map.find(t);
			}

			auto end() const {
				return m_map.end();
			}

			rynx::unordered_map<T, rynx::type_index::virtual_type, local_hash_function<T>> m_map;
		};
	}

	using ecs_table_create_func = rynx::function<rynx::unique_ptr<rynx::ecs_internal::itable>()>;

	template<typename T>
	ecs_table_create_func make_ecs_table_create_func(type_id_t type_id) {
		return [type_id]() {
			if constexpr (std::is_empty_v<T>) {
				return rynx::unique_ptr<rynx::ecs_internal::itable>(nullptr); // don't create tables for empty types (tag types)
			}
			else if constexpr(std::is_base_of_v<rynx::ecs_no_component_tag, T>)
				return rynx::unique_ptr<rynx::ecs_internal::itable>(nullptr); // no need to be able to create tables for types that user has promised to not use as components
			else {
				return rynx::make_unique<rynx::ecs_internal::component_table<T>>(type_id);
			}
		};
	}

	using id = rynx::ecs_internal::id;
}