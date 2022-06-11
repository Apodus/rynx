
#include <rynx/tech/reflection.hpp>

namespace rynx::reflection::internal {
	registration_object* global_linked_list_initializer_head = nullptr;
}


namespace {
	uint64_t hashmix(uint64_t key) {
		key = (~key) + (key << 21); // key = (key << 21) - key - 1;
		key = key ^ (key >> 24);
		key = (key + (key << 3)) + (key << 8); // key * 265
		key = key ^ (key >> 14);
		key = (key + (key << 2)) + (key << 4); // key * 21
		key = key ^ (key >> 28);
		key = key + (key << 31);
		return key;
	}
}

void rynx::reflection::reflections::load_generated_reflections() {
	auto* current = rynx::reflection::internal::global_linked_list_initializer_head;
	while (current != nullptr) {
		current->registration_function(*this);
		current = current->next;
	}

	for (auto&& t : m_reflections) {
		std::sort(t.second.m_fields.begin(), t.second.m_fields.end());
	}
}

uint64_t rynx::reflection::reflections::hash(std::string typeName) {
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


rynx::reflection::type& rynx::reflection::reflections::get(const rynx::reflection::field& f) {
	auto it = m_reflections.find(f.m_type_name);
	if (it == m_reflections.end()) {
		rynx::reflection::type t;
		// t.m_type_name = "!!" + f.m_type_name;
		t.m_type_name = f.m_type_name;
		it = m_reflections.emplace(f.m_type_name, std::move(t)).first;
	}
	return it->second;
}

const rynx::reflection::type& rynx::reflection::reflections::get(const rynx::reflection::field& f) const {
	return const_cast<reflections*>(this)->get(f);
}

rynx::reflection::type* rynx::reflection::reflections::find(uint64_t typeId) {
	for (auto&& entry : m_reflections) {
		if (entry.second.m_type_index_value == typeId) {
			return &entry.second;
		}
	}
	return nullptr;
}

rynx::reflection::type* rynx::reflection::reflections::find(std::string typeName) {
	for (auto&& entry : m_reflections) {
		if (entry.second.m_type_name == typeName) {
			return &entry.second;
		}
	}
	return nullptr;
}

const rynx::reflection::type* rynx::reflection::reflections::find(uint64_t typeId) const {
	return const_cast<reflections*>(this)->find(typeId);
}

const rynx::reflection::type* rynx::reflection::reflections::find(std::string typeName) const {
	return const_cast<reflections*>(this)->find(typeName);
}

std::vector<std::pair<std::string, rynx::reflection::type>> rynx::reflection::reflections::get_reflection_data() const {
	std::vector<std::pair<std::string, rynx::reflection::type>> result;
	for (const auto& entry : m_reflections) {
		result.emplace_back(entry.first, entry.second);
	}
	return result;
}

bool rynx::reflection::reflections::has(const std::string& s) const { return m_reflections.find(s) != m_reflections.end(); }
