
#pragma once

#include <rynx/filesystem/filetree/node.hpp>

namespace rynx::filesystem::filetree {
	class memoryfile_node : public node {
	public:
		memoryfile_node(const rynx::string& name);
		memoryfile_node(const rynx::string& name, rynx::filesystem::memory_file file);

		virtual bool file_exists(const rynx::string& path) const override;
		virtual bool directory_exists(const rynx::string& path) const override;

		virtual rynx::shared_ptr<rynx::filesystem::iwrite_file> open_write(const rynx::string& path, filesystem::iwrite_file::mode mode) override;
		virtual rynx::shared_ptr<rynx::filesystem::iread_file> open_read(const rynx::string& path) override;

		virtual bool remove(const rynx::string&) override { return false; }
		virtual std::vector<rynx::string> enumerate_content(const rynx::string& path,
			rynx::filesystem::recursive,
			rynx::filesystem::filetree::detail::enumerate_flags flags) override;
	
	private:
		rynx::filesystem::memory_file m_file;
	};
}