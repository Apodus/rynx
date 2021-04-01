#pragma once

#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/type_index.hpp>
#include <rynx/tech/ecs/table.hpp>
#include <rynx/tech/serialization.hpp>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <typeinfo>

// TODO: move this somewhere
static uint64_t hashmix(uint64_t key)
{
	key = (~key) + (key << 21); // key = (key << 21) - key - 1;
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); // key * 265
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); // key * 21
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
}

namespace rynx {
	class itable;

	namespace reflection {
		class reflections;

		class field {
		public:
			std::string m_field_name;
			std::string m_type_name;
			int32_t m_memory_offset = 0;
			int32_t m_memory_size = 0;
			std::vector<std::string> m_annotations;

			template<typename MemberType, typename ObjectType>
			static rynx::reflection::field construct(std::string fieldName, MemberType ObjectType::* ptr, std::vector<std::string> fieldAnnotations = {}) {
				rynx::reflection::field f;
				f.m_memory_offset = static_cast<int32_t>(reinterpret_cast<uint64_t>(&((*static_cast<ObjectType*>(nullptr)).*ptr)));
				f.m_memory_size = sizeof(MemberType);
				f.m_type_name = typeid(MemberType).name();
				f.m_field_name = fieldName;
				f.m_annotations = std::move(fieldAnnotations);
				return f;
			}

			bool operator < (const field& other) const {
				return m_memory_offset < other.m_memory_offset;
			}
		};
	}

	namespace serialization {
		template<> struct Serialize<rynx::reflection::field> {
			template<typename IOStream>
			void serialize(const rynx::reflection::field& f, IOStream& writer) {
				rynx::serialize(f.m_field_name, writer);
				rynx::serialize(f.m_type_name, writer);
				rynx::serialize(f.m_memory_offset, writer);
				rynx::serialize(f.m_memory_size, writer);
				rynx::serialize(f.m_annotations, writer);
			}

			template<typename IOStream>
			void deserialize(rynx::reflection::field& f, IOStream& reader) {
				rynx::deserialize(f.m_field_name, reader);
				rynx::deserialize(f.m_type_name, reader);
				rynx::deserialize(f.m_memory_offset, reader);
				rynx::deserialize(f.m_memory_size, reader);
				rynx::deserialize(f.m_annotations, reader);
			}
		};
	}

	namespace reflection {
		class type {
		public:
			std::string m_type_name;
			std::vector<field> m_fields;
			int32_t m_type_index_value = -1;
			bool m_is_type_segregated = false;

			rynx::ecs_table_create_func m_create_table_func;
			std::function<opaque_unique_ptr<void>()> m_create_instance_func;
			std::function<std::unique_ptr<rynx::ecs_internal::ivalue_segregation_map>()> m_create_map_func;

			template<typename MemberType, typename ObjectType>
			type& add_field(std::string fieldName, MemberType ObjectType::* ptr) {
				auto field = rynx::reflection::field::construct(fieldName, ptr);
				bool exists_already = false;
				for (auto&& f : m_fields) {
					exists_already |= (field.m_memory_offset == f.m_memory_offset);
				}
				if (!exists_already) {
					m_fields.emplace_back(field);
				}
				return *this;
			}

			template<typename MemberType, typename ObjectType>
			type& add_field(std::string fieldName, MemberType ObjectType::* ptr, std::vector<std::string> fieldAnnotations) {
				m_fields.emplace_back(rynx::reflection::field::construct(fieldName, ptr, fieldAnnotations));
				return *this;
			}
		};
	}

	namespace serialization {
		template<> struct Serialize<rynx::reflection::type> {
			template<typename IOStream>
			void serialize(const rynx::reflection::type& t, IOStream& writer) {
				rynx::serialize(t.m_type_name, writer);
				rynx::serialize(t.m_fields, writer);
				// note: type index value should not be serialized
				//       because the values can change over different runs.
			}

			template<typename IOStream>
			void deserialize(rynx::reflection::type& t, IOStream& reader) {
				rynx::deserialize(t.m_type_name, reader);
				rynx::deserialize(t.m_fields, reader);
			}
		};
	}

	namespace reflection {

		template <typename T>
		class make_reflect {
		public:
			rynx::reflection::type operator()(rynx::type_index& typeIndex) {
				rynx::reflection::type result{ std::string(typeid(T).name()) };
				result.m_type_index_value = static_cast<int32_t>(typeIndex.id<T>());
				return result;
			}
		};

		namespace internal {
			struct registration_object;

			extern registration_object* global_linked_list_initializer_head;
			extern registration_object* global_linked_list_initializer_tail;

			struct registration_object {
				registration_object(std::function<void(rynx::reflection::reflections&)> op) {
					registration_function = std::move(op);

					if (global_linked_list_initializer_tail == nullptr) {
						global_linked_list_initializer_head = this;
						global_linked_list_initializer_tail = this;
					}
					else {
						global_linked_list_initializer_tail->next = this;
						global_linked_list_initializer_tail = this;
					}
				}

				registration_object* next = nullptr;
				std::function<void(rynx::reflection::reflections&)> registration_function;
			};
		}

		class reflections {
		public:
			reflections(rynx::type_index& index) : m_type_index(index) {}

			void load_generated_reflections() {
				auto* current = rynx::reflection::internal::global_linked_list_initializer_head;
				while (current != nullptr) {
					current->registration_function(*this);
					current = current->next;
				}

				for (auto&& t : m_reflections) {
					std::sort(t.second.m_fields.begin(), t.second.m_fields.end());
				}
			}

			uint64_t hash(std::string typeName) {
				auto* typeReflection = find(typeName);
				
				auto hash_type_reflection = [&](rynx::reflection::type* t, auto& self) -> uint64_t {
					size_t typeHash = std::hash<std::string>()(t->m_type_name);
					for (auto&& field : t->m_fields) {
						uint64_t fieldNameHash = std::hash<std::string>()(field.m_field_name);
						uint64_t fieldTypeNameHash = std::hash<std::string>()(field.m_type_name);
						
						uint64_t fieldHash = hashmix(fieldTypeNameHash + fieldNameHash);
						fieldHash = hashmix(fieldHash + field.m_memory_offset);
						fieldHash = hashmix(fieldHash + field.m_memory_size);
						
						auto* fieldTypeReflection = find(field.m_type_name);
						if (fieldTypeReflection) {
							fieldHash = hashmix(fieldHash + self(fieldTypeReflection, self));
						}

						typeHash ^= fieldHash; // must be insensitive to order changes
					}
					
					return typeHash;
				};
				
				return hash_type_reflection(typeReflection, hash_type_reflection);
			}

			template<typename T>
			rynx::reflection::type& create() {
				rynx::reflection::type result;
				result.m_type_name = std::string(typeid(T).name());
				result.m_type_index_value = static_cast<int32_t>(m_type_index.id<T>());
				
				// TODO: Ecs utils should be tightly knit somewhere under ecs. It is wrong for reflection to make these assumptions.
				result.m_is_type_segregated = std::is_base_of_v<rynx::ecs_value_segregated_component_tag, std::remove_cvref_t<T>>;
				result.m_create_table_func = rynx::make_ecs_table_create_func<T>(result.m_type_index_value);
				result.m_create_instance_func = []() {
					return opaque_unique_ptr<void>(new T(), [](void* t) { if(t) delete static_cast<T*>(t); });
				};

				result.m_create_map_func = []() {
					using tag_t = rynx::ecs_value_segregated_component_tag;
					using type_t = std::remove_cvref_t<T>;
					if constexpr (std::is_base_of_v<tag_t, type_t> && !std::is_same_v<tag_t, type_t>) {
						return std::unique_ptr<rynx::ecs_internal::ivalue_segregation_map>(new rynx::ecs_internal::value_segregation_map<T>());
					}
					rynx_assert(false, "calling map create for a type that has no map type");
					return std::unique_ptr<rynx::ecs_internal::ivalue_segregation_map>(nullptr);
				};

				m_reflections.emplace(std::string(typeid(T).name()), std::move(result));
				return get<T>();
			}

			template<typename T>
			rynx::reflection::type& get() {
				auto it = m_reflections.find(typeid(T).name());
				if (it == m_reflections.end()) {
					rynx::reflection::type& reflection = create<T>();
					reflection.m_type_name = "!!" + reflection.m_type_name;
					return reflection;
				}
				return it->second;
			}

			rynx::reflection::type& get(const rynx::reflection::field& f) {
				auto it = m_reflections.find(f.m_type_name);
				if (it == m_reflections.end()) {
					rynx::reflection::type t;
					t.m_type_name = "!!" + f.m_type_name;
					it = m_reflections.emplace(f.m_type_name, std::move(t)).first;
				}
				return it->second;
			}

			rynx::reflection::type* find(uint64_t typeId) {
				for (auto&& entry : m_reflections) {
					if (entry.second.m_type_index_value == typeId) {
						return &entry.second;
					}
				}
				return nullptr;
			}

			rynx::reflection::type* find(std::string typeName) {
				for (auto&& entry : m_reflections) {
					if (entry.second.m_type_name == typeName) {
						return &entry.second;
					}
				}
				return nullptr;
			}

			std::vector<std::pair<std::string, rynx::reflection::type>> get_reflection_data() const {
				std::vector<std::pair<std::string, rynx::reflection::type>> result;
				for (const auto& entry : m_reflections) {
					result.emplace_back(entry.first, entry.second);
				}
				return result;
			}

			bool has(const std::string& s) { return m_reflections.find(s) != m_reflections.end(); }
			template<typename T> bool has() { return has(typeid(T).name()); }

		private:
			rynx::type_index& m_type_index;
			rynx::unordered_map<std::string, rynx::reflection::type> m_reflections;

			// allow serialization to access our data
			friend struct rynx::serialization::Serialize<rynx::reflection::reflections>;
		};
	}

	namespace serialization {
		template<> struct Serialize<rynx::reflection::reflections> {
			template<typename IOStream>
			void serialize(const rynx::reflection::reflections& t, IOStream& writer) {
				rynx::serialize(t.m_reflections, writer);
			}

			template<typename IOStream>
			void deserialize(rynx::reflection::reflections& t, IOStream& reader) {
				rynx::deserialize(t.m_reflections, reader);
			}
		};
	}
}