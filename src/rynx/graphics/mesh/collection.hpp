#pragma once

#include <rynx/graphics/mesh/id.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/math/geometry/polygon.hpp>
#include <rynx/tech/std/memory.hpp>
#include <rynx/tech/std/string.hpp>

namespace rynx {
	namespace graphics {
		class mesh;
		class GPUTextures;

		struct mesh_entry {
			rynx::string name;
			rynx::graphics::mesh_id id;
		};

		class mesh_collection {
		public:
			mesh_collection(rynx::shared_ptr<rynx::graphics::GPUTextures> gpuTextures);

			std::vector<mesh_entry> getListOfMeshes() const;

			mesh* get(mesh_id id);
			mesh_id findByName(rynx::string humanReadableName);

			mesh_id create_transient(rynx::unique_ptr<mesh> mesh);
			mesh_id create_transient(rynx::polygon shape);

			mesh_id create(rynx::unique_ptr<mesh> mesh);
			mesh_id create(rynx::polygon shape);
			mesh_id create(rynx::string human_readable_name, rynx::unique_ptr<mesh> mesh);
			mesh_id create(rynx::string human_readable_name, rynx::polygon shape);
			void erase(mesh_id id);

			void save_all_meshes_to_disk(rynx::string path);
			void save_mesh_to_disk(mesh_id, rynx::string path);
			void load_all_meshes_from_disk(rynx::string path);
			void clear();

		private:
			mesh_id generate_mesh_id();

			rynx::shared_ptr<rynx::graphics::GPUTextures> m_pGpuTextures;
			rynx::unordered_map<mesh_id, rynx::unique_ptr<mesh>> m_storage;
		};
	}
}