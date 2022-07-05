
#pragma once

#include <rynx/filesystem/filetree/node.hpp>

namespace rynx::filesystem::filetree {
	class native_directory_node : public node {
	private:
		rynx::string m_native_fs_path;
	public:
		native_directory_node(const rynx::string& name, const rynx::string& native_path);
		virtual bool file_exists(const rynx::string& path) const override;
		virtual bool directory_exists(const rynx::string& path) const override;
		virtual rynx::shared_ptr<rynx::filesystem::iwrite_file> open_write(const rynx::string& path, filesystem::iwrite_file::mode mode) override;
		virtual rynx::shared_ptr<rynx::filesystem::iread_file> open_read(const rynx::string& path) override;
		virtual bool remove(const rynx::string& path) override;

		virtual std::vector<rynx::string> enumerate_content(
			const rynx::string& path,
			rynx::filesystem::recursive recurse,
			rynx::filesystem::filetree::detail::enumerate_flags flags) override;
	};
}