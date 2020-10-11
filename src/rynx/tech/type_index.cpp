
#include <rynx/tech/type_index.hpp>

alignas(std::hardware_destructive_interference_size) std::atomic<uint64_t> rynx::type_index::runningTypeIndex_global = 0;

