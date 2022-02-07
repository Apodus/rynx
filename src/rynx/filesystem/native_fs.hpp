#pragma once

#include <string>
#include <vector>

namespace rynx::filesystem
{
	enum class recursive {
		yes,
		no
	};

	namespace native
	{
		std::string absolute_path(const std::string& path);
		std::string relative_path(const std::string& path);
		bool file_exists(const std::string& path);
		bool directory_exists(const std::string &path);
		
		bool create_directory(const std::string &path);
		bool delete_directory(const std::string &path);
		bool delete_file(const std::string& path);

		std::vector<std::string> enumerate(
			const std::string& directory,
			recursive = recursive::no);

		std::vector<std::string> enumerate_files(
			const std::string& directory,
			recursive = recursive::no);

		std::vector<std::string> enumerate_directories(
			const std::string& directory,
			recursive = recursive::no);
	}
}