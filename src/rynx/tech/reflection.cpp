
#include <rynx/tech/reflection.hpp>

namespace rynx::reflection::internal {
	registration_object* global_linked_list_initializer_head = nullptr;
	registration_object* global_linked_list_initializer_tail = nullptr;
}