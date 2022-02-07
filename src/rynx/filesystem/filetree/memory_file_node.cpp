
#include <rynx/filesystem/filetree/native_file_node.hpp>

rynx::filesystem::filetree::native_file_node::native_file_node(std::shared_ptr<node> parent, const std::string& name, const std::string& native_path)
	: node(parent, name) {
	m_native_fs_path = native_path;
	if (m_native_fs_path.back() == '/')
		m_native_fs_path.pop_back();
}

bool rynx::filesystem::filetree::native_file_node::file_exists(const std::string& path) const {
	if(path.empty())
		return rynx::filesystem::native::file_exists(m_native_fs_path);
	return false;
}

bool rynx::filesystem::filetree::native_file_node::directory_exists(const std::string&) const {
	return false;
}

std::shared_ptr<rynx::filesystem::iwrite_file> rynx::filesystem::filetree::native_file_node::open_write(const std::string& path, filesystem::iwrite_file::mode mode) {
	if (path.empty())
		return std::make_shared<rynx::filesystem::nativefile_write>(m_native_fs_path, mode);
	return {};
}

std::shared_ptr<rynx::filesystem::iread_file> rynx::filesystem::filetree::native_file_node::open_read(const std::string& path) {
	if (path.empty())
		if (rynx::filesystem::native::file_exists(m_native_fs_path))
			return std::make_shared<rynx::filesystem::nativefile_read>(m_native_fs_path);
	return {};
}

std::vector<std::string> rynx::filesystem::filetree::native_file_node::files_in_directory(
	const std::string& path,
	rynx::filesystem::recursive) {
	if (path.empty())
		return { name() };
	return {};
}