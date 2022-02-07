
#pragma once

#include <rynx/filesystem/filetree/node.hpp>

namespace rynx::filesystem::filetree {
	class native_file_node : public node {
	private:
		std::string m_native_fs_path;

	public:
		native_file_node(const std::string& name, const std::string& native_path);
		virtual bool file_exists(const std::string& path) const override;
		virtual bool directory_exists(const std::string& path) const override;
		virtual std::shared_ptr<rynx::filesystem::iwrite_file> open_write(const std::string& path, filesystem::iwrite_file::mode mode) override;
		virtual std::shared_ptr<rynx::filesystem::iread_file> open_read(const std::string& path) override;
		virtual bool remove(const std::string& path) override;

		virtual std::vector<std::string> enumerate_content(
			const std::string& path,
			rynx::filesystem::recursive recurse,
			rynx::filesystem::filetree::detail::enumerate_flags flags) override;
	};
}