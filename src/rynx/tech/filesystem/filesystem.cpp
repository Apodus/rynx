
#include <rynx/tech/filesystem/filesystem.hpp>
#include <rynx/system/assert.hpp>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
	std::string to_std(const rynx::string& str) {
		return std::string(str.data(), str.size());
	}

	rynx::string to_rynx(const std::string& str) {
		return rynx::string(str.data(), str.size());
	}
}

namespace rynx {
	namespace filesystem {

		void write_file(rynx::string path, std::span<const char> data) {
			std::ofstream out(to_std(path), std::ios::binary);
			out.write(&data.front(), data.size());
		}

		std::vector<char> read_file(rynx::string path) {
			if (!std::filesystem::exists(to_std(path)))
				return {};
			auto size = std::filesystem::file_size(to_std(path));
			std::vector<char> result;
			result.resize(size);
			std::ifstream in(to_std(path), std::ios::binary);
			in.read(result.data(), result.size());
			return result;
		}

		rynx::string resolve(rynx::string path) {
			return rynx::filesystem::path_normalize(to_rynx(std::filesystem::relative(to_std(path)).string()));
		}

		void create_directory(rynx::string path) {
			std::filesystem::create_directory(to_std(path));
		}

		bool is_directory(rynx::string path) {
			return std::filesystem::is_directory(to_std(path));
		}

		rynx::string path_normalize(rynx::string path) {
			for (auto& c : path)
				if (c == '\\')
					c = '/';
			
			if (rynx::filesystem::is_directory(path))
				if (path.back() != '/')
					path.push_back('/');

			return path;
		}

		std::pair<rynx::string, rynx::string> path_relative_folder(rynx::string filepath, rynx::string start) {
			rynx::string file_relative_path = rynx::filesystem::path_normalize(to_rynx(std::filesystem::relative(to_std(filepath)).string()));
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

			rynx::string file_folder = file_relative_path.substr(0, pos + 1);
			rynx::string file_name = file_relative_path.substr(pos + 1);
			return { file_folder, file_name };
		}

		std::vector<rynx::string> list_recursive(rynx::string path) {
			std::vector<rynx::string> result;
			for (auto entry : std::filesystem::recursive_directory_iterator(to_std(path))) {
				if (entry.is_directory()) {
					auto [directorypath, directoryname] = path_relative_folder(to_rynx(entry.path().relative_path().string()));
					result.emplace_back(directorypath + directoryname);
				}
				if (entry.is_regular_file()) {
					auto [filepath, filename] = path_relative_folder(to_rynx(entry.path().relative_path().string()));
					result.emplace_back(filepath + filename);
				}
			}
			return result;
		}

		std::vector<rynx::string> list_files(rynx::string path) {
			std::vector<rynx::string> result;
			for (auto entry : std::filesystem::directory_iterator(to_std(path))) {
				if (entry.is_regular_file()) {
					auto [filepath, filename] = path_relative_folder(to_rynx(entry.path().relative_path().string()));
					result.emplace_back(rynx::filesystem::path_normalize(filepath + filename));
				}
			}
			return result;
		}

		std::vector<rynx::string> list_directories(rynx::string path) {
			std::vector<rynx::string> result;
			for (auto entry : std::filesystem::directory_iterator(to_std(path))) {
				if (entry.is_directory()) {
					auto [directorypath, directoryname] = path_relative_folder(to_rynx(entry.path().relative_path().string()));
					result.emplace_back(rynx::filesystem::path_normalize(directorypath + directoryname));
				}
			}
			return result;
		}
	}
}
