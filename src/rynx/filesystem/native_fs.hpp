#pragma once

#include <rynx/tech/std/string.hpp>
#include <vector>

namespace rynx::filesystem
{
	enum class recursive {
		yes,
		no
	};

	rynx::string resolve_path(rynx::string path);

	namespace native
	{
		rynx::string absolute_path(const rynx::string& path);
		rynx::string relative_path(const rynx::string& path);
		bool file_exists(const rynx::string& path);
		bool directory_exists(const rynx::string &path);
		
		bool create_directory(const rynx::string &path);
		bool delete_directory(const rynx::string &path);
		bool delete_file(const rynx::string& path);

		std::vector<rynx::string> enumerate(
			const rynx::string& directory,
			recursive = recursive::no);

		std::vector<rynx::string> enumerate_files(
			const rynx::string& directory,
			recursive = recursive::no);

		std::vector<rynx::string> enumerate_directories(
			const rynx::string& directory,
			recursive = recursive::no);
	}
}