
#include <rynx/filesystem/filetree/memfile_directory_node.hpp>

rynx::filesystem::filetree::memory_directory_node::memory_directory_node(const rynx::string& name) : node(name) {
}

bool rynx::filesystem::filetree::memory_directory_node::file_exists(const rynx::string& path) const {
	return m_files.find(path) != m_files.end();
}

bool rynx::filesystem::filetree::memory_directory_node::directory_exists(const rynx::string& path) const {
	for (auto&& pathFile : m_files) {
		if (pathFile.first.starts_with(path))
			return true;
	}
	return false;
}

rynx::shared_ptr<rynx::filesystem::iwrite_file> rynx::filesystem::filetree::memory_directory_node::open_write(const rynx::string& path, filesystem::iwrite_file::mode mode) {
	size_t offset = 0;
	while (true) {
		auto next = path.find_first_of('/', offset);
		if (next == rynx::string::npos) {
			break;
		}

		m_directories.emplace(path.substr(0, next));
		offset = next + 1;
	}

	auto it = m_files.find(path);
	if (it != m_files.end())
		return rynx::make_shared<rynx::filesystem::memoryfile_write>(*(it->second), mode);
	auto created_file = m_files.emplace(path, rynx::make_shared<rynx::filesystem::memory_file>());
	return rynx::make_shared<rynx::filesystem::memoryfile_write>(*created_file.first->second);
}

rynx::shared_ptr<rynx::filesystem::iread_file> rynx::filesystem::filetree::memory_directory_node::open_read(const rynx::string& path) {
	auto it = m_files.find(path);
	if (it != m_files.end())
		return rynx::make_shared<rynx::filesystem::memoryfile_read>(*(it->second));
	return {};
}

bool rynx::filesystem::filetree::memory_directory_node::remove(const rynx::string& path) {
	auto it = m_files.find(path);
	if (it != m_files.end()) {
		m_files.erase(it);
		return true;
	}
	return false;
}

std::vector<rynx::string> rynx::filesystem::filetree::memory_directory_node::enumerate_content(
	const rynx::string& path,
	rynx::filesystem::recursive recurse,
	rynx::filesystem::filetree::detail::enumerate_flags flags) {
	std::vector<rynx::string> results;
	if (path.empty()) {
		results.reserve(m_files.size() + m_directories.size() + 1);
		if (flags.directories())
			results.emplace_back();
		if (recurse == recursive::no) {
			if (flags.files()) {
				for (auto&& memfile_path : m_files) {
					if (memfile_path.first.find_first_of('/') == rynx::string::npos)
						results.emplace_back(memfile_path.first);
				}
			}
			if (flags.directories()) {
				for (auto&& internal_path : m_directories) {
					if (internal_path.find_first_of('/') == rynx::string::npos)
						results.emplace_back(internal_path + "/");
				}
			}
		}
		else {
			if(flags.files())
				for (auto&& memfile_path : m_files)
					results.emplace_back(memfile_path.first);
			if(flags.directories())
				for (auto&& internal_path : m_directories)
					results.emplace_back(internal_path + "/");
		}
		return results;
	}

	if (flags.files()) {
		for (auto&& internal_path : m_files) {
			if (internal_path.first.starts_with(path))
				if (recurse == rynx::filesystem::recursive::yes || internal_path.first.find_first_of('/', path.length() + 1) == rynx::string::npos)
					results.emplace_back(internal_path.first.substr(path.length() + 1));
		}
	}
	if (flags.directories()) {
		for (auto&& internal_path : m_directories) {
			if (internal_path.starts_with(path) && internal_path.length() > path.length() + 1)
				if (recurse == rynx::filesystem::recursive::yes || internal_path.find_first_of('/', path.length() + 1) == rynx::string::npos)
					results.emplace_back(internal_path.substr(path.length() + 1) + "/");
		}
	}
	return results;
}