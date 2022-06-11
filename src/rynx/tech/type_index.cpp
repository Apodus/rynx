
#include <rynx/tech/type_index.hpp>
#include <algorithm>
#include <vector>

namespace {
	uint64_t runningTypeIndex_global = 0;
}

rynx::type_index::used_type* rynx::type_index::internal::used_type_list = nullptr;

void rynx::type_index::initialize() {
	used_type* current = rynx::type_index::internal::used_type_list;
	if (current == nullptr)
		return;

	std::vector<used_type> types;
	while (current) { types.emplace_back(*current); current = current->next; }
	std::sort(types.begin(), types.end(), [](const used_type& a, const used_type& b) { return std::strcmp(a.name, b.name) > 0; });
	char const* current_name = types.front().name;
	uint64_t currentId = runningTypeIndex_global;
	for (auto&& type : types) {
		if (strcmp(current_name, type.name) != 0) {
			currentId = ++runningTypeIndex_global;
			current_name = type.name;
		}
		*type.id = currentId;
	}

	// prevent reinitialization of processed values
	rynx::type_index::internal::used_type_list = nullptr;
}


rynx::type_index::virtual_type rynx::type_index::create_virtual_type() {
	return virtual_type{ runningTypeIndex_global++ };
}