
#include <rynx/tech/ecs/scenes.hpp>
#include <rynx/tech/serialization.hpp>
#include <random>

rynx::scene_id rynx::scene_id::generate() {
	scene_id id;
	std::random_device gen;
	id.m_random_1 = uint64_t(gen()) | (uint64_t(gen()) << uint64_t(32));;
	id.m_random_2 = uint64_t(gen()) | (uint64_t(gen()) << uint64_t(32));
	return id;
}

rynx::scene_id::operator bool() const { return (m_random_1 | m_random_2) != 0; }

bool rynx::scene_id::operator < (const scene_id& other) const {
	return std::tie(m_random_1, m_random_2) < std::tie(other.m_random_1, other.m_random_2);
}

rynx::scene_id::operator rynx::string() const {
	static constexpr char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	auto value_to_base64_str = [](uint64_t v) {
		rynx::string result;
		for (int bits_covered = 0; bits_covered < 64; bits_covered += 6) {
			result += base64_chars[v & ((1 << 6) - 1)];
			v >>= 6;
		}
		return result;
	};

	return value_to_base64_str(m_random_1) + value_to_base64_str(m_random_2);
}



void rynx::scenes::scan_directory(rynx::filesystem::vfs& fs, const rynx::string& logical_path) {
	auto files = fs.enumerate_files(logical_path, rynx::filesystem::recursive::yes);
	for (auto&& filepath : files) {
		bool success = true;
		std::vector<char> data;
		{
			auto reader = fs.open_read(filepath);
			auto result = read_definition(*reader);
			success = result.first;
			if (result.first) {
				internal_update(result.second, filepath);
			}
			else {
				reader->seek_beg(0);
				data = reader->read_all();
			}
		}

		if (!success && !data.empty()) {
			logmsg("converting %s to new scene format", filepath.c_str());
			auto id = scene_id::generate();
			rynx::scene_info info;
			info.id = id;
			info.ui_path = filepath;
			info.name = filepath;

			auto file = fs.open_write(filepath);
			rynx::serialize(serialized_scene_marker, *file);
			rynx::serialize(info, *file);
			rynx::serialize(data, *file);

			internal_update(info, filepath);
		}
	}
}

std::vector<char> rynx::scenes::get(rynx::filesystem::vfs& fs, rynx::scene_id id) const {
	auto it = m_filepaths.find(id);
	auto file = fs.open_read(it->second);
	read_definition(*file);
	return rynx::deserialize<std::vector<char>>(*file);
}

std::vector<char> rynx::scenes::get(rynx::filesystem::vfs& fs, rynx::string filepath) const {
	for (auto&& entry : m_filepaths) {
		if (entry.second == filepath) {
			auto file = fs.open_read(entry.second);
			read_definition(*file);
			return rynx::deserialize<std::vector<char>>(*file);
		}
	}
	return std::vector<char>{};
}

void rynx::scenes::save_scene(
	rynx::filesystem::vfs& fs,
	std::vector<char>& serialized_scene,
	rynx::string ui_path,
	rynx::string scene_name,
	rynx::string filepath
) {
	if (fs.file_exists(filepath)) {
		// overwrite existing scene
		rynx::scene_info info;
		{
			auto old_file = fs.open_read(filepath);
			auto def = read_definition(*old_file);
			if (def.first) {
				info = def.second;
			}
		}

		if (info.id) {
			auto file = fs.open_write(filepath);
			rynx::serialize(serialized_scene_marker, *file);
			rynx::serialize(info, *file);
			rynx::serialize(serialized_scene, *file);

			internal_update(info, filepath);
		}
		else {
			// uh oh.. save failed. should probably communicate this back to user hehe.
		}
	}
	else {
		// create a new scene
		auto id = scene_id::generate();
		rynx::scene_info info;
		info.id = id;
		info.ui_path = ui_path;
		info.name = scene_name;

		auto storageFilePath = filepath + "_" + id.operator rynx::string() + ".rynxscene";
		auto file = fs.open_write(storageFilePath);
		rynx::serialize(serialized_scene_marker, *file);
		rynx::serialize(info, *file);
		rynx::serialize(serialized_scene, *file);

		internal_update(info, storageFilePath);
	}
}

rynx::scene_info& rynx::scenes::filepath_to_info(const rynx::string& path) {
	auto it = m_filepath_to_info.find(path);
	rynx_assert(it != m_filepath_to_info.end(), "filepath to info must succeed");
	return it->second;
}

const rynx::scene_info& rynx::scenes::filepath_to_info(const rynx::string& path) const {
	auto it = m_filepath_to_info.find(path);
	rynx_assert(it != m_filepath_to_info.end(), "filepath to info must succeed");
	return it->second;
}

std::vector<std::pair<rynx::string, rynx::scene_id>> rynx::scenes::list_scenes() const {
	std::vector<std::pair<rynx::string, rynx::scene_id>> result;
	for (auto&& entry : m_filepaths) {
		result.emplace_back(entry.second, entry.first);
	}
	std::sort(result.begin(), result.end(), [](const std::pair<rynx::string, rynx::scene_id>& a, const std::pair<rynx::string, rynx::scene_id>& b) {
		return a.first < b.first;
		});
	return result;
}

void rynx::scenes::internal_update(rynx::scene_info& info, rynx::string filepath) {
	m_infos.emplace(info.id, info);
	m_filepaths[info.id] = filepath;
	m_filepath_to_info[filepath] = info;
}