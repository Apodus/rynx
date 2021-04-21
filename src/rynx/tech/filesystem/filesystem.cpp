
#include <rynx/tech/filesystem/filesystem.hpp>
#include <rynx/system/assert.hpp>
#include <filesystem>
#include <fstream>

namespace rynx {
	namespace filesystem {

		void write_file(std::string path, std::span<const char> data) {
			std::ofstream out(path, std::ios::binary);
			out.write(&data.front(), data.size());
		}

		std::vector<char> read_file(std::string path) {
			auto size = std::filesystem::file_size(path);
			std::vector<char> result;
			result.resize(size);
			std::ifstream in(path, std::ios::binary);
			in.read(result.data(), result.size());
			return result;
		}

		std::string path_normalize(std::string path) {
			for (auto& c : path) {
				if (c == '\\') {
					c = '/';
				}
			}
			return path;
		}

		std::pair<std::string, std::string> path_relative_folder(std::string filepath, std::string start) {
			std::string file_relative_path = rynx::filesystem::path_normalize(std::filesystem::relative(filepath).string());
			rynx_assert(file_relative_path.starts_with(start), "relative path must begin with given path");
			auto start_pos = file_relative_path.find(start);
			start = rynx::filesystem::path_normalize(start);
			if (!start.empty()) {
				if (start.back() == '/') {
					start.pop_back();
				}
				file_relative_path = file_relative_path.substr(start_pos + start.length());
			}
			auto pos = file_relative_path.find_last_of('/');

			std::string file_folder = file_relative_path.substr(0, pos + 1);
			std::string file_name = file_relative_path.substr(pos + 1);
			return { file_folder, file_name };
		}
	}
}