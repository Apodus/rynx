
#pragma once

#include <vector>
#include <string>
#include <span>

namespace rynx {
	namespace filesystem {
		void write_file(std::string path, std::span<const char> data);
		std::vector<char> read_file(std::string path);

		std::string path_normalize(std::string path);
		std::pair<std::string, std::string> path_relative_folder(std::string filepath, std::string start = "");
	}
}
