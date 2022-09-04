
#pragma once

#include <rynx/ecs/components.hpp>

namespace rynx {
	class ecs;
}

namespace rynx::ecs_detail {
	class scene_serializer {
		rynx::ecs* host = nullptr;
	public:
		scene_serializer(rynx::ecs* host_ptr) : host(host_ptr) {}
		scene_serializer(rynx::ecs& host_ref) : host(&host_ref) {}

		// TODO: Move elsewhere
		// finds the parent scene link of a given entity.
		// if no scene parent is found, returns invalid id.
		id scene_parent_of(id entity);

		std::vector<rynx::components::scene::persistent_id> persistent_id_path_create(rynx::id to);
		std::vector<rynx::components::scene::persistent_id> persistent_id_path_create(rynx::id from, rynx::id to);
		rynx::id persistent_id_path_find(rynx::entity_range_t range, rynx::id from, const std::vector<rynx::components::scene::persistent_id>& path);
		rynx::id persistent_id_path_find(const std::vector<rynx::components::scene::persistent_id>& path);

		// serializes the top-most scene to memory.
		void serialize_scene(
			rynx::reflection::reflections& reflections,
			rynx::filesystem::vfs& vfs,
			rynx::scenes& scenes,
			rynx::serialization::vector_writer& out);

		rynx::serialization::vector_writer serialize_scene(
			rynx::reflection::reflections& reflections,
			rynx::filesystem::vfs& vfs,
			rynx::scenes& scenes);

		// deserializes a scene and any sub-scenes referenced in it.
		// returns an entity range of ids in scene (excluding subscenes)
		// and an entity range of all ids deserialized (including subscenes)
		std::tuple<entity_range_t, entity_range_t> deserialize_scene(
			rynx::reflection::reflections& reflections,
			rynx::filesystem::vfs& vfs,
			rynx::scenes& scenes,
			rynx::components::transform::position scene_pos,
			rynx::serialization::vector_reader& in);

		entity_range_t load_subscenes(rynx::reflection::reflections& reflections, rynx::filesystem::vfs& vfs, rynx::scenes& scenes);
	};
}
