#pragma once

#include <rynx/graphics/mesh/id.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/math/geometry/polygon.hpp>
#include <memory>

namespace rynx {
	namespace graphics {
		class mesh;
		class GPUTextures;

		struct mesh_entry {
			std::string name;
			rynx::graphics::mesh_id id;
		};

		class mesh_collection {
		public:
			mesh_collection(std::shared_ptr<rynx::graphics::GPUTextures> gpuTextures);

			std::vector<mesh_entry> getListOfMeshes() const;

			mesh* get(mesh_id id);
			mesh_id findByName(std::string humanReadableName);

			mesh_id create_transient(std::unique_ptr<mesh> mesh);
			mesh_id create_transient(rynx::polygon shape);

			mesh_id create(std::unique_ptr<mesh> mesh);
			mesh_id create(rynx::polygon shape);
			mesh_id create(std::string human_readable_name, std::unique_ptr<mesh> mesh);
			mesh_id create(std::string human_readable_name, rynx::polygon shape);
			void erase(mesh_id id);

			void save_all_meshes_to_disk(std::string path);
			void save_mesh_to_disk(mesh_id, std::string path);
			void load_all_meshes_from_disk(std::string path);
			void clear();

		private:
			mesh_id generate_mesh_id();

			std::shared_ptr<rynx::graphics::GPUTextures> m_pGpuTextures;
			rynx::unordered_map<mesh_id, std::unique_ptr<mesh>> m_storage;
		};
	}
}