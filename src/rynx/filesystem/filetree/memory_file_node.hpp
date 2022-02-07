
#pragma once

#include <rynx/filesystem/filetree/node.hpp>

namespace rynx::filesystem::filetree {
	class memoryfile_node : public node {
	public:
		memoryfile_node(const std::string& name);
		memoryfile_node(const std::string& name, rynx::filesystem::memory_file file);

		virtual bool file_exists(const std::string& path) const override;
		virtual bool directory_exists(const std::string& path) const override;

		virtual std::shared_ptr<rynx::filesystem::iwrite_file> open_write(const std::string& path, filesystem::iwrite_file::mode mode) override;
		virtual std::shared_ptr<rynx::filesystem::iread_file> open_read(const std::string& path) override;

		virtual bool remove(const std::string&) override { return false; }
		virtual std::vector<std::string> enumerate_content(const std::string& path,
			rynx::filesystem::recursive,
			rynx::filesystem::filetree::detail::enumerate_flags flags) override;
	
	private:
		rynx::filesystem::memory_file m_file;
	};
}