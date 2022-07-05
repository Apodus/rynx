
#include <rynx/filesystem/filetree/native_directory_node.hpp>
#include <rynx/filesystem/native_fs.hpp>
#include <rynx/system/assert.hpp>

rynx::filesystem::filetree::native_directory_node::native_directory_node(const rynx::string& name, const rynx::string& native_path)
	: node(name) {
	m_native_fs_path = native_path;
	m_native_fs_path = rynx::filesystem::native::relative_path(m_native_fs_path);
	rynx_assert(rynx::filesystem::native::directory_exists(m_native_fs_path), "mounting native directory, but directory does not exist");
	if (m_native_fs_path.back() != '/')
		m_native_fs_path += "/";
}

bool rynx::filesystem::filetree::native_directory_node::file_exists(const rynx::string& path) const {
	rynx::string native_path = m_native_fs_path + path;
	return rynx::filesystem::native::file_exists(native_path);
}

bool rynx::filesystem::filetree::native_directory_node::directory_exists(const rynx::string& path) const {
	rynx::string native_path = m_native_fs_path + path;
	return rynx::filesystem::native::directory_exists(native_path);
}

rynx::shared_ptr<rynx::filesystem::iwrite_file> rynx::filesystem::filetree::native_directory_node::open_write(const rynx::string& path, filesystem::iwrite_file::mode mode) {
	rynx::string native_path = m_native_fs_path + path;
	return rynx::make_shared<rynx::filesystem::nativefile_write>(native_path, mode);
}

rynx::shared_ptr<rynx::filesystem::iread_file> rynx::filesystem::filetree::native_directory_node::open_read(const rynx::string& path) {
	rynx::string native_path = m_native_fs_path + path;
	if (rynx::filesystem::native::file_exists(native_path))
		return rynx::make_shared<rynx::filesystem::nativefile_read>(native_path);
	return rynx::shared_ptr<rynx::filesystem::iread_file>();
}

bool rynx::filesystem::filetree::native_directory_node::remove(const rynx::string& path) {
	return rynx::filesystem::native::delete_file(m_native_fs_path + path);
}


std::vector<rynx::string> rynx::filesystem::filetree::native_directory_node::enumerate_content(
	const rynx::string& path,
	rynx::filesystem::recursive recurse,
	rynx::filesystem::filetree::detail::enumerate_flags flags) {
	auto res = [&]() {
		if(flags.directories() & flags.files())
			return rynx::filesystem::native::enumerate(m_native_fs_path + path, recurse);
		else if(flags.files())
			return rynx::filesystem::native::enumerate_files(m_native_fs_path + path, recurse);
		return rynx::filesystem::native::enumerate_directories(m_native_fs_path + path, recurse);
	}();
	
	size_t erase_count = m_native_fs_path.size();
	if (m_native_fs_path.starts_with("./"))
		erase_count -= 2;

	std::for_each(
		res.begin(),
		res.end(),
		[total_erase_count = erase_count + path.size()](rynx::string& fileName) {
		if (fileName[total_erase_count] == '/')
			fileName.replace(0, total_erase_count + 1, "");
		else
			fileName.replace(0, total_erase_count, "");
		}
	);

	if(flags.directories() && rynx::filesystem::native::directory_exists(m_native_fs_path + path))
		res.emplace_back("/");
	return res;
}
