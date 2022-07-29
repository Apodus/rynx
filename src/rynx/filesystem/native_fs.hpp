#pragma once

#include <rynx/std/string.hpp>
#include <vector>

namespace rynx::filesystem
{
	enum class recursive {
		yes,
		no
	};

	FileSystemDLL rynx::string resolve_path(rynx::string path);

	namespace native
	{
		FileSystemDLL rynx::string absolute_path(const rynx::string& path);
		FileSystemDLL rynx::string relative_path(const rynx::string& path);
		FileSystemDLL bool file_exists(const rynx::string& path);
		FileSystemDLL bool directory_exists(const rynx::string &path);
		
		FileSystemDLL bool create_directory(const rynx::string &path);
		FileSystemDLL bool delete_directory(const rynx::string &path);
		FileSystemDLL bool delete_file(const rynx::string& path);

		FileSystemDLL std::vector<rynx::string> enumerate(
			const rynx::string& directory,
			recursive = recursive::no);

		FileSystemDLL std::vector<rynx::string> enumerate_files(
			const rynx::string& directory,
			recursive = recursive::no);

		FileSystemDLL std::vector<rynx::string> enumerate_directories(
			const rynx::string& directory,
			recursive = recursive::no);
	}
}