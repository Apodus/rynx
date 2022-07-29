#pragma once

#include <rynx/filesystem/filetree/node.hpp>
#include <rynx/std/unordered_map.hpp>
#include <rynx/std/unordered_set.hpp>

namespace rynx::filesystem::filetree {
	class memory_directory_node : public node {
	public:
		memory_directory_node(const rynx::string& name);
		virtual bool file_exists(const rynx::string& path) const override;
		virtual bool directory_exists(const rynx::string& path) const override;
		virtual rynx::shared_ptr<rynx::filesystem::iwrite_file> open_write(const rynx::string& path, filesystem::iwrite_file::mode mode) override;
		virtual rynx::shared_ptr<rynx::filesystem::iread_file> open_read(const rynx::string& path) override;
		virtual bool remove(const rynx::string& path) override;

		virtual std::vector<rynx::string> enumerate_content(
			const rynx::string& path,
			rynx::filesystem::recursive recurse,
			rynx::filesystem::filetree::detail::enumerate_flags) override;
	private:
		rynx::unordered_map<rynx::string, rynx::shared_ptr<rynx::filesystem::memory_file>> m_files;
		rynx::unordered_set<rynx::string> m_directories;
	};
}