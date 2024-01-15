#pragma once

#include <rynx/graphics/mesh/id.hpp>
#include <rynx/std/unordered_map.hpp>
#include <rynx/math/geometry/polygon.hpp>
#include <rynx/std/memory.hpp>
#include <rynx/std/string.hpp>

namespace rynx {
	namespace graphics {
		class mesh;
		class GPUTextures;

		struct mesh_entry {
			rynx::string name;
			rynx::graphics::mesh_id id;
		};

		class GraphicsDLL mesh_collection {
		public:
			mesh_collection(rynx::shared_ptr<rynx::graphics::GPUTextures> gpuTextures);

			std::vector<rynx::graphics::mesh_entry> getListOfMeshes() const;

			rynx::graphics::mesh* get(rynx::graphics::mesh_id id);
			rynx::graphics::mesh_id findByName(rynx::string humanReadableName);

			rynx::graphics::mesh_id create_transient(rynx::unique_ptr<rynx::graphics::mesh> mesh);
			
			rynx::graphics::mesh_id create_transient(rynx::polygon shape);
			rynx::graphics::mesh_id create_transient_boundary(rynx::polygon shape, float lineWidth);

			rynx::graphics::mesh_id create(rynx::unique_ptr<rynx::graphics::mesh> mesh);
			rynx::graphics::mesh_id create(rynx::string human_readable_name, rynx::unique_ptr<rynx::graphics::mesh> mesh);

			rynx::graphics::mesh_id create(rynx::polygon shape);
			rynx::graphics::mesh_id create(rynx::string human_readable_name, rynx::polygon shape);
			
			rynx::graphics::mesh_id create_boundary(rynx::polygon shape, float lineWidth);
			rynx::graphics::mesh_id create_boundary(rynx::string human_readable_name, rynx::polygon shape, float lineWidth);

			void erase(rynx::graphics::mesh_id id);

			void save_all_meshes_to_disk(rynx::string path);
			void save_mesh_to_disk(rynx::graphics::mesh_id, rynx::string path);
			void load_all_meshes_from_disk(rynx::string path);
			void clear();

		private:
			rynx::graphics::mesh_id generate_mesh_id();

			rynx::shared_ptr<rynx::graphics::GPUTextures> m_pGpuTextures;
			rynx::unordered_map<rynx::graphics::mesh_id, rynx::unique_ptr<rynx::graphics::mesh>> m_storage;
		};
	}
}