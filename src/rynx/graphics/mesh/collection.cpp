
#include <rynx/graphics/mesh/collection.hpp>
#include <rynx/graphics/mesh/mesh.hpp>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/tech/filesystem/filesystem.hpp>
#include <rynx/std/serialization.hpp>
#include <rynx/graphics/mesh/polygon_triangulation.hpp>
#include <chrono>
#include <filesystem>


rynx::graphics::mesh_collection::mesh_collection(rynx::shared_ptr<rynx::graphics::GPUTextures> gpuTextures) : m_pGpuTextures(gpuTextures) {
	auto default_mesh = rynx::polygon_triangulation().make_mesh(rynx::Shape::makeBox(1.0f));
	default_mesh->scale(sqrt(2.0f));
	default_mesh->build();
	default_mesh->set_transient();
	m_storage.emplace(mesh_id(), std::move(default_mesh));
}

rynx::graphics::mesh_id rynx::graphics::mesh_collection::findByName(rynx::string humanReadableName) {
	for (auto&& entry : m_storage) {
		if (entry.second->humanReadableId == humanReadableName)
			return entry.first;
	}
	return {};
}

void rynx::graphics::mesh_collection::save_all_meshes_to_disk(rynx::string path) {
	if (!std::filesystem::exists(path.c_str())) {
		std::filesystem::create_directory(path.c_str());
	}
	for (auto&& entry : m_storage) {
		save_mesh_to_disk(entry.first, path);
	}
}

void rynx::graphics::mesh_collection::save_mesh_to_disk(mesh_id id, rynx::string path) {
	auto mesh_it = m_storage.find(id);
	if (mesh_it->second->is_transient())
		return;

	rynx::serialization::vector_writer mem_out;
	rynx::serialize(id, mem_out);
	rynx_assert(mesh_it != m_storage.end(), "trying to save mesh that does not exist");
	rynx::serialize(*mesh_it->second, mem_out);

	if (path.back() != '/')
		path.push_back('/');

	rynx::string filename = mesh_it->second->humanReadableId + "_" + rynx::to_string(id.value) + ".rynxmesh";
	rynx::string filepath = path + mesh_it->second->get_category() + filename;
	logmsg("saving mesh to '%s'", filepath.c_str());
	rynx::filesystem::write_file(filepath, mem_out.data());
}

void rynx::graphics::mesh_collection::load_all_meshes_from_disk(rynx::string path) {
	std::filesystem::recursive_directory_iterator it(path.c_str());
	for (auto&& entry : it) {
		if (entry.is_directory()) {
			// TODO: should we have different folders for different kinds of meshes?
		}
		if (entry.is_regular_file()) {
			auto [file_folder, file_name] = rynx::filesystem::path_relative_folder(entry.path().string().c_str(), path);
			logmsg("path '%s', filename '%s'", file_folder.c_str(), file_name.c_str());

			if (entry.path().string().ends_with(".rynxmesh")) {
				std::vector<char> data = rynx::filesystem::read_file(entry.path().string().c_str());
				rynx::serialization::vector_reader in(data);
				auto mesh_id = rynx::deserialize<rynx::graphics::mesh_id>(in);
				auto mesh_p = rynx::make_unique<rynx::graphics::mesh>();
				mesh_p->set_category(file_folder);
				rynx::deserialize<rynx::graphics::mesh>(*mesh_p, in);
				m_storage.emplace(mesh_id, std::move(mesh_p));
			}
		}
	}
}

void rynx::graphics::mesh_collection::clear() {
	m_storage.clear();
}


rynx::graphics::mesh_id rynx::graphics::mesh_collection::generate_mesh_id() {
	return { std::chrono::high_resolution_clock::now().time_since_epoch().count() };
}

std::vector<rynx::graphics::mesh_entry> rynx::graphics::mesh_collection::getListOfMeshes() const {
	std::vector<mesh_entry> mesh_names;
	for (auto&& entry : this->m_storage) {
		// transients can't be shown outside. if user picks a transient and saves the level, loading will crash.
		// this is because transients get a new id every run.
		if (!entry.second->is_transient()) {
			mesh_names.emplace_back(entry.second->humanReadableId, entry.first);
		}
	}
	return mesh_names;
}

rynx::graphics::mesh* rynx::graphics::mesh_collection::get(rynx::graphics::mesh_id id) {
	auto it = m_storage.find(id);
	rynx_assert(it != m_storage.end(), "mesh not found");
	return it->second.get();
}

rynx::graphics::mesh_id rynx::graphics::mesh_collection::create_transient(rynx::unique_ptr<rynx::graphics::mesh> mesh) {
	auto id = create(std::move(mesh));
	m_storage.find(id)->second->set_transient();
	return id;
}

rynx::graphics::mesh_id rynx::graphics::mesh_collection::create_transient(rynx::polygon shape) {
	auto id = create(std::move(shape));
	m_storage.find(id)->second->set_transient();
	return id;
}

rynx::graphics::mesh_id rynx::graphics::mesh_collection::create(rynx::unique_ptr<mesh> mesh) {
	mesh_id id = generate_mesh_id();
	auto it = m_storage.emplace(id, std::move(mesh));
	it.first->second->build();
	it.first->second->id = id;
	it.first->second->humanReadableId = "anonymous";
	return id;
}

rynx::graphics::mesh_id rynx::graphics::mesh_collection::create(rynx::polygon shape) {
	return create(rynx::polygon_triangulation().make_mesh(shape, { 0, 0, 1, 1 }));
}

rynx::graphics::mesh_id rynx::graphics::mesh_collection::create(rynx::string human_readable_name, rynx::unique_ptr<rynx::graphics::mesh> mesh) {
	auto id = create(std::move(mesh));
	m_storage.find(id)->second->humanReadableId = human_readable_name;
	return id;
}

rynx::graphics::mesh_id rynx::graphics::mesh_collection::create(rynx::string human_readable_name, rynx::polygon shape) {
	auto id = create(std::move(shape));
	m_storage.find(id)->second->humanReadableId = human_readable_name;
	return id;
}

void rynx::graphics::mesh_collection::erase(mesh_id id) {
	m_storage.erase(id);
}
