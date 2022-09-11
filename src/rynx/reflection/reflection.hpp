#pragma once

#include <rynx/std/unordered_map.hpp>
#include <rynx/std/type_index.hpp>
#include <rynx/ecs/table.hpp>
#include <rynx/std/serialization_declares.hpp>

#include <algorithm>
#include <vector>
#include <typeinfo>

namespace rynx {
	
	namespace reflection {
		class reflections;

		class field {
		public:
			rynx::string m_field_name;
			rynx::string m_type_name;
			int32_t m_memory_offset = 0;
			int32_t m_memory_size = 0;
			std::vector<rynx::string> m_annotations;

			template<typename MemberType, typename ObjectType>
			static rynx::reflection::field construct(rynx::string fieldName, MemberType ObjectType::* ptr, std::vector<rynx::string> fieldAnnotations = {}) {
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
			rynx::string m_type_name;
			std::vector<field> m_fields;
			int32_t m_type_index_value = -1;
			bool m_is_type_segregated = false;
			bool m_serialization_allowed = true;
			
			rynx::ecs_table_create_func m_create_table_func;
			rynx::function<opaque_unique_ptr<void>()> m_create_instance_func;
			rynx::function<opaque_unique_ptr<void>(const std::vector<char>&)> m_deserialize_instance_func;
			rynx::function<rynx::unique_ptr<rynx::ecs_internal::ivalue_segregation_map>()> m_create_map_func;

			template<typename MemberType, typename ObjectType>
			type& add_field(rynx::string fieldName, MemberType ObjectType::* ptr) {
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
			type& add_field(rynx::string fieldName, MemberType ObjectType::* ptr, std::vector<rynx::string> fieldAnnotations) {
				for (auto&& field : m_fields)
					if (field.m_field_name == fieldName)
						return *this;
				
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
			rynx::reflection::type operator()() {
				rynx::reflection::type result{ rynx::string(typeid(T).name()) };
				result.m_type_index_value = static_cast<int32_t>(rynx::type_index::id<T>());
				return result;
			}
		};

		namespace internal {
			struct registration_object;

			ReflectionDLL extern registration_object* global_linked_list_initializer_head;

			struct registration_object {
				registration_object(rynx::function<void(rynx::reflection::reflections&)> op) {
					registration_function = std::move(op);
					next = global_linked_list_initializer_head;
					global_linked_list_initializer_head = this;
				}

				registration_object* next = nullptr;
				rynx::function<void(rynx::reflection::reflections&)> registration_function;
			};
		}

		class ReflectionDLL reflections {
		public:
			reflections() = default;

			void load_generated_reflections();
			uint64_t hash(rynx::string typeName);

			rynx::reflection::type& get(const rynx::reflection::field& f);
			const rynx::reflection::type& get(const rynx::reflection::field& f) const;

			rynx::reflection::type* find(uint64_t typeId);
			rynx::reflection::type* find(rynx::string typeName);

			const rynx::reflection::type* find(uint64_t typeId) const;
			const rynx::reflection::type* find(rynx::string typeName) const;

			std::vector<std::pair<rynx::string, rynx::reflection::type>> get_reflection_data() const;
			bool has(const rynx::string& s) const;
			
			template<typename T> bool has() const { return has(typeid(T).name()); }

			template<typename T>
			rynx::reflection::type& create() {
				{
					auto it = m_reflections.find(rynx::string(typeid(T).name()));
					if (it != m_reflections.end())
						return it->second;
				}
				
				rynx::reflection::type result;
				result.m_type_name = rynx::string(typeid(T).name());
				result.m_type_index_value = static_cast<int32_t>(rynx::type_index::id<T>());
				result.m_serialization_allowed = !std::is_base_of_v<rynx::ecs_no_serialize_tag, T>;

				rynx::unique_ptr<rynx::function<void(void*)>> a = rynx::make_unique<rynx::function<void(void*)>>([](void* a) { delete static_cast<int*>(a); });
				rynx::unique_ptr<rynx::function<void(void*)>> b = rynx::make_unique<rynx::function<void(void*)>>([](void* b) { delete static_cast<int*>(b); });
				a = std::move(b);

				// TODO: Ecs utils should be tightly knit somewhere under ecs. It is wrong for reflection to make these assumptions.
				result.m_is_type_segregated = std::is_base_of_v<rynx::ecs_value_segregated_component_tag, std::remove_cvref_t<T>>;
				result.m_create_table_func = rynx::make_ecs_table_create_func<T>(result.m_type_index_value);
				result.m_create_instance_func = []() {
					return opaque_unique_ptr<void>(new T(), [](void* t) { if (t) delete static_cast<T*>(t); });
				};

				result.m_create_map_func = []() {
					using tag_t = rynx::ecs_value_segregated_component_tag;
					using type_t = std::remove_cvref_t<T>;
					if constexpr (std::is_base_of_v<tag_t, type_t> && !std::is_same_v<tag_t, type_t>) {
						return rynx::unique_ptr<rynx::ecs_internal::ivalue_segregation_map>(new rynx::ecs_internal::value_segregation_map<T>());
					}
					else {
						rynx_assert(false, "calling map create for a type that has no map type");
						return rynx::unique_ptr<rynx::ecs_internal::ivalue_segregation_map>(nullptr);
					}
				};

				result.m_deserialize_instance_func = []([[maybe_unused]] const std::vector<char>& serialized) {
					if constexpr (std::is_empty_v<T>) {
						return opaque_unique_ptr<void>(new T(), [](void* t) { if (t) delete static_cast<T*>(t); });
					}
					else {
						rynx::serialization::vector_reader in(serialized);
						T* obj = new T();
						rynx::deserialize(*obj, in);
						return opaque_unique_ptr<void>(obj, [](void* t) { if (t) delete static_cast<T*>(t); });
					}
				};

				m_reflections.emplace(rynx::string(typeid(T).name()), std::move(result));
				return get<T>();
			}

			template<typename T>
			rynx::reflection::type& get() {
				auto it = m_reflections.find(typeid(T).name());
				if (it == m_reflections.end()) {
					rynx::reflection::type& reflection = create<T>();
					return reflection;
				}
				return it->second;
			}

		private:
			rynx::unordered_map<rynx::string, rynx::reflection::type> m_reflections;

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