
#pragma once

#include <vector>
#include <string>
#include <span>

namespace rynx {
	namespace filesystem {
		void write_file(std::string path, std::span<const char> data);
		std::vector<char> read_file(std::string path);
		std::string resolve(std::string path);

		void create_directory(std::string path);
		bool is_directory(std::string path);

		std::string path_normalize(std::string path);
		std::pair<std::string, std::string> path_relative_folder(std::string filepath, std::string start = "");
		std::vector<std::string> list_recursive(std::string path); // returns files & directories
		std::vector<std::string> list_files(std::string path);
		std::vector<std::string> list_directories(std::string path);
	}
}
