
#pragma once

#include <rynx/tech/std/string.hpp>
#include <vector>
#include <span>

namespace rynx {
	namespace filesystem {
		void write_file(rynx::string path, std::span<const char> data);
		std::vector<char> read_file(rynx::string path);
		rynx::string resolve(rynx::string path);

		void create_directory(rynx::string path);
		bool is_directory(rynx::string path);

		rynx::string path_normalize(rynx::string path);
		std::pair<rynx::string, rynx::string> path_relative_folder(rynx::string filepath, rynx::string start = "");
		std::vector<rynx::string> list_recursive(rynx::string path); // returns files & directories
		std::vector<rynx::string> list_files(rynx::string path);
		std::vector<rynx::string> list_directories(rynx::string path);
	}
}
