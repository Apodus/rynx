
#pragma once

#include <rynx/tech/serialization.hpp>
#include <rynx/tech/ecs/id.hpp>
#include <rynx/system/assert.hpp>
#include <vector>
#include <memory>
#include <numeric>
#include <functional>

namespace rynx {

	using type_id_t = uint64_t;
	using entity_id_t = uint64_t;
	using index_t = uint32_t;

	namespace ecs_internal {

		class itable {
		public:
			itable(uint64_t type_id) : m_type_id(type_id) {}
			virtual ~itable() {}
			virtual void erase(entity_id_t entityId) = 0;

			virtual void serialize(rynx::serialization::vector_writer& writer) = 0;
			virtual void deserialize(rynx::serialization::vector_reader& reader) = 0;
			virtual void for_each_id_field(std::function<void(rynx::id&)>) = 0;

			// when we already have content in ecs, and we deserialize (load) more content in,
			// we need to operate over all deserialized entities but to not touch pre-existing data.
			virtual void for_each_id_field_from_index(size_t startingIndex, std::function<void(rynx::id&)>) = 0;

			virtual void swap_adjacent_indices_for(const std::vector<index_t>& index_points) = 0;
			virtual void swap_to_given_order(const std::vector<index_t>& relative_sort_order) = 0;

			virtual void* get(index_t) = 0;

			virtual void copyTableTypeTo(type_id_t typeId, std::vector<std::unique_ptr<itable>>& targetTables) = 0;
			virtual void moveFromIndexTo(index_t index, itable* dst) = 0;

			uint64_t m_type_id;
		};

		template<typename T>
		class component_table : public itable {
		public:
			component_table(uint64_t type_id) : itable(type_id) {}

			virtual ~component_table() {}

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

			virtual void serialize(rynx::serialization::vector_writer& writer) {
				rynx::serialize(m_data, writer);
			}

			virtual void deserialize(rynx::serialization::vector_reader& reader) {
				rynx::deserialize(m_data, reader);
			}

			// TODO: skip iterating over data if no types in hierarchy have id members.
			virtual void for_each_id_field(std::function<void(rynx::id&)> op) {
				for (auto& entry : m_data) {
					rynx::for_each_id_field(entry, op);
				}
			}

			// TODO: skip iterating over data if no types in hierarchy have id members.
			virtual void for_each_id_field_from_index(size_t startingIndex, std::function<void(rynx::id&)> op) {
				for (size_t i = startingIndex; i < m_data.size(); ++i) {
					rynx::for_each_id_field(m_data[i], op);
				}
			}

			virtual void* get(index_t i) override { return &m_data[i]; }

			virtual void erase(entity_id_t id) override {
				m_data[id] = std::move(m_data.back());
				m_data.pop_back();
			}

			// NOTE: This is required for moving entities between categories reliably.
			virtual void copyTableTypeTo(type_id_t typeId, std::vector<std::unique_ptr<itable>>& targetTables) override {
				if (typeId >= targetTables.size()) {
					targetTables.resize(((3 * typeId) >> 1) + 1);
				}
				targetTables[typeId] = std::make_unique<component_table>(typeId);
			}

			virtual void moveFromIndexTo(index_t index, itable* dst) override {
				T moved(std::move(m_data[index]));
				m_data[index] = std::move(m_data.back());
				static_cast<component_table*>(dst)->emplace_back(std::move(moved));
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
	}

	using id = rynx::ecs_internal::id;
}