
#pragma once

#include <rynx/ecs/id.hpp>

namespace rynx {
	class ecs;

	namespace reflection {
		class reflections;
	}

	namespace serialization {
		class vector_writer;
		class vector_reader;
	}
}

namespace rynx::ecs_detail {
	class raw_serializer {
		rynx::ecs* host = nullptr;
		
	public:
		raw_serializer(rynx::ecs* host_ptr) : host(host_ptr) {}
		raw_serializer(rynx::ecs& host_ref) : host(&host_ref) {}

		void serialize(rynx::reflection::reflections& reflections, rynx::serialization::vector_writer& out);
		rynx::serialization::vector_writer serialize(rynx::reflection::reflections& reflections);
		entity_range_t deserialize(rynx::reflection::reflections& reflections, rynx::serialization::vector_reader& in);
	};
}
