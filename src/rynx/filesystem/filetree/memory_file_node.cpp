
#include <rynx/filesystem/filetree/memory_file_node.hpp>

rynx::filesystem::filetree::memoryfile_node::memoryfile_node(const rynx::string& name) : node(name) {}
rynx::filesystem::filetree::memoryfile_node::memoryfile_node(const rynx::string& name, rynx::filesystem::memory_file file)
	: node(name) {
	m_file = std::move(file);
}

bool rynx::filesystem::filetree::memoryfile_node::file_exists(const rynx::string& path) const {
	return path.empty();
}

bool rynx::filesystem::filetree::memoryfile_node::directory_exists(const rynx::string&) const {
	return false;
}

rynx::shared_ptr<rynx::filesystem::iwrite_file> rynx::filesystem::filetree::memoryfile_node::open_write(const rynx::string& path, filesystem::iwrite_file::mode mode) {
	if (path.empty())
		return rynx::make_shared<memoryfile_write>(m_file, mode);
	return {};
}

rynx::shared_ptr<rynx::filesystem::iread_file> rynx::filesystem::filetree::memoryfile_node::open_read(const rynx::string& path) {
	if (path.empty())
		return rynx::make_shared<rynx::filesystem::memoryfile_read>(m_file);
	return {};
}

std::vector<rynx::string> rynx::filesystem::filetree::memoryfile_node::enumerate_content(
	const rynx::string& path,
	rynx::filesystem::recursive,
	rynx::filesystem::filetree::detail::enumerate_flags flags) {
	if (path.empty() && flags.files())
		return { name() };
	return {};
}
