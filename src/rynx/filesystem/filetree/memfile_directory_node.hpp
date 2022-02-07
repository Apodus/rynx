#pragma once

#include <rynx/filesystem/filetree/node.hpp>
#include <unordered_set>

namespace rynx::filesystem::filetree {
	class memory_directory_node : public node {
	public:
		memory_directory_node(const std::string& name);
		virtual bool file_exists(const std::string& path) const override;
		virtual bool directory_exists(const std::string& path) const override;
		virtual std::shared_ptr<rynx::filesystem::iwrite_file> open_write(const std::string& path, filesystem::iwrite_file::mode mode) override;
		virtual std::shared_ptr<rynx::filesystem::iread_file> open_read(const std::string& path) override;
		virtual bool remove(const std::string& path) override;

		virtual std::vector<std::string> enumerate_content(
			const std::string& path,
			rynx::filesystem::recursive recurse,
			rynx::filesystem::filetree::detail::enumerate_flags) override;
	private:
		std::unordered_map<std::string, std::shared_ptr<rynx::filesystem::memory_file>> m_files;
		std::unordered_set<std::string> m_directories;
	};
}