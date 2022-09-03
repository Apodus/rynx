
#include <rynx/std/type_index.hpp>
#include <algorithm>
#include <vector>

namespace {
	rynx::type_index::type_id_t runningTypeIndex_global = 0;
	rynx::type_index::used_type* all_accessed_types_list_for_debug = nullptr;
}

rynx::type_index::used_type* used_type_list = nullptr;

rynx::type_index::used_type* rynx::type_index::internal::get_used_type_list() {
	return used_type_list;
}

void rynx::type_index::internal::set_used_type_list(rynx::type_index::used_type* v) {
	used_type_list = v;
}

void rynx::type_index::initialize() {
	used_type* current = used_type_list;
	if (current == nullptr)
		return;

	std::vector<used_type> types;
	while (current) { types.emplace_back(*current); current = current->next; }
	std::sort(types.begin(), types.end(), [](const used_type& a, const used_type& b) { return std::strcmp(a.name, b.name) > 0; });
	char const* current_name = types.front().name;
	type_id_t currentId = runningTypeIndex_global;
	for (auto&& type : types) {
		if (strcmp(current_name, type.name) != 0) {
			currentId = ++runningTypeIndex_global;
			current_name = type.name;
		}
		*type.id = currentId;
	}

	// prevent reinitialization of processed values
	all_accessed_types_list_for_debug = used_type_list;
	used_type_list = nullptr;
}


rynx::type_index::virtual_type rynx::type_index::create_virtual_type() {
	return virtual_type{ runningTypeIndex_global++ };
}

const char* rynx::type_index::name_of_type(rynx::type_index::type_id_t id) {
	used_type* current = all_accessed_types_list_for_debug;
	while (current) {
		if (*current->id == id)
			return current->name;
		current = current->next;
	}
	return "type not found";
}