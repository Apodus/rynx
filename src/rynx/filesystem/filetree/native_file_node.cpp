
#include <rynx/filesystem/filetree/native_file_node.hpp>

rynx::filesystem::filetree::native_file_node::native_file_node(const rynx::string& name, const rynx::string& native_path)
	: node(name) {
	m_native_fs_path = native_path;
	if (m_native_fs_path.back() == '/')
		m_native_fs_path.pop_back();
}

bool rynx::filesystem::filetree::native_file_node::file_exists(const rynx::string& path) const {
	if(path.empty())
		return rynx::filesystem::native::file_exists(m_native_fs_path);
	return false;
}

bool rynx::filesystem::filetree::native_file_node::directory_exists(const rynx::string&) const {
	return false;
}

rynx::shared_ptr<rynx::filesystem::iwrite_file> rynx::filesystem::filetree::native_file_node::open_write(const rynx::string& path, filesystem::iwrite_file::mode mode) {
	if (path.empty())
		return rynx::make_shared<rynx::filesystem::nativefile_write>(m_native_fs_path, mode);
	return {};
}

rynx::shared_ptr<rynx::filesystem::iread_file> rynx::filesystem::filetree::native_file_node::open_read(const rynx::string& path) {
	if (path.empty())
		if (rynx::filesystem::native::file_exists(m_native_fs_path))
			return rynx::make_shared<rynx::filesystem::nativefile_read>(m_native_fs_path);
	return {};
}

bool rynx::filesystem::filetree::native_file_node::remove(const rynx::string& path) {
	if (path.empty()) {
		return rynx::filesystem::native::delete_file(m_native_fs_path);
	}
	return false;
}

std::vector<rynx::string> rynx::filesystem::filetree::native_file_node::enumerate_content(
	const rynx::string& path,
	rynx::filesystem::recursive,
	rynx::filesystem::filetree::detail::enumerate_flags flags) {
	if (path.empty() && flags.files() && rynx::filesystem::native::file_exists(m_native_fs_path))
		return { "" };
	return {};
}
