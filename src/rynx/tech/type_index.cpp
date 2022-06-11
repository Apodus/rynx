
#include <rynx/tech/type_index.hpp>
#include <algorithm>
#include <vector>

uint64_t rynx::type_index::runningTypeIndex_global = 0;
rynx::type_index::used_type* rynx::type_index::used_type_list = nullptr;

rynx::type_index::type_index() {
	used_type* current = used_type_list;
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
	used_type_list = nullptr;
}
