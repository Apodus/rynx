
#pragma once

#include <rynx/std/string.hpp>
#include <vector>
#include <span>

namespace rynx {
	namespace filesystem {
		TechDLL void write_file(rynx::string path, std::span<const char> data);
		TechDLL std::vector<char> read_file(rynx::string path);
		TechDLL rynx::string resolve(rynx::string path);

		TechDLL void create_directory(rynx::string path);
		TechDLL bool is_directory(rynx::string path);

		TechDLL rynx::string path_normalize(rynx::string path);
		TechDLL std::pair<rynx::string, rynx::string> path_relative_folder(rynx::string filepath, rynx::string start = "");
		TechDLL std::vector<rynx::string> list_recursive(rynx::string path); // returns files & directories
		TechDLL std::vector<rynx::string> list_files(rynx::string path);
		TechDLL std::vector<rynx::string> list_directories(rynx::string path);
	}
}
