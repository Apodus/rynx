
#include <rynx/filesystem/filetree/node.hpp>
#include <rynx/filesystem/filekinds/compressedfile.hpp>

rynx::filesystem::filetree::node::node(const rynx::string& name) {
	m_name = name;
}

rynx::string rynx::filesystem::filetree::node::full_vfs_path() const {
	auto parent = m_parent.lock();
	if (parent)
		return parent->full_vfs_path() + "/" + m_name;
	return m_name;
}

rynx::shared_ptr<rynx::filesystem::iwrite_file> rynx::filesystem::filetree::node::open_write(
	const rynx::string&, // path
	rynx::filesystem::iwrite_file::mode) {
	return {};
}

bool rynx::filesystem::filetree::node::remove(const rynx::string&) {
	return false;
}

rynx::shared_ptr<rynx::filesystem::iread_file> rynx::filesystem::filetree::node::open_read_with_settings(const rynx::string& virtual_path) {
	auto file_ptr = open_read(virtual_path);
	if (!m_compress_files)
		return file_ptr;
	return rynx::make_shared<rynx::filesystem::compressedfile_read>(file_ptr);
}

rynx::shared_ptr<rynx::filesystem::iwrite_file> rynx::filesystem::filetree::node::open_write_with_settings(const rynx::string& virtual_path, rynx::filesystem::iwrite_file::mode mode) {
	auto file_ptr = open_write(virtual_path, mode);
	if (!m_compress_files)
		return file_ptr;
	return rynx::make_shared<rynx::filesystem::compressedfile_write>(file_ptr);
}

rynx::shared_ptr<rynx::filesystem::iread_file> rynx::filesystem::filetree::node::open_read(const rynx::string&) {
	return {};
}

bool rynx::filesystem::filetree::node::file_exists(const rynx::string&) const { return false; }
bool rynx::filesystem::filetree::node::directory_exists(const rynx::string&) const { return false; }
bool rynx::filesystem::filetree::node::exists(const rynx::string& path) const { return file_exists(path) || directory_exists(path); }

std::vector<rynx::string> rynx::filesystem::filetree::node::enumerate_content(
	const rynx::string&,
	rynx::filesystem::recursive,
	rynx::filesystem::filetree::detail::enumerate_flags) {
	return {};
}

bool rynx::filesystem::filetree::node::is_file_mount() const {
	return file_exists("");
}