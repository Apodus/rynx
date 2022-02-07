
#include <rynx/filesystem/filetree/memory_file_node.hpp>

rynx::filesystem::filetree::memoryfile_node::memoryfile_node(const std::string& name) : node(name) {}
rynx::filesystem::filetree::memoryfile_node::memoryfile_node(const std::string& name, rynx::filesystem::memory_file file)
	: node(name) {
	m_file = std::move(file);
}

bool rynx::filesystem::filetree::memoryfile_node::file_exists(const std::string& path) const {
	return path.empty();
}

bool rynx::filesystem::filetree::memoryfile_node::directory_exists(const std::string&) const {
	return false;
}

std::shared_ptr<rynx::filesystem::iwrite_file> rynx::filesystem::filetree::memoryfile_node::open_write(const std::string& path, filesystem::iwrite_file::mode mode) {
	if (path == "")
		return std::make_shared<memoryfile_write>(m_file, mode);
	return {};
}

std::shared_ptr<rynx::filesystem::iread_file> rynx::filesystem::filetree::memoryfile_node::open_read(const std::string& path) {
	if (path.empty())
		return std::make_shared<rynx::filesystem::memoryfile_read>(m_file);
	return {};
}

std::vector<std::string> rynx::filesystem::filetree::memoryfile_node::enumerate_content(
	const std::string& path,
	rynx::filesystem::recursive,
	rynx::filesystem::filetree::detail::enumerate_flags flags) {
	if (path.empty() && flags.files())
		return { name() };
	return {};
}
