
#include <rynx/filesystem/virtual_filesystem.hpp>
#include <rynx/system/assert.hpp>
#include <filesystem>

namespace {
	std::string path_normalize(std::string path) {
		bool skipNext = false;
		for (auto& c : path) {
			if (c == ':') {
				skipNext = true;
			}
			else if (!skipNext && c == '\\') {
				c = '/';
			}
			else {
				skipNext = false;
			}
		}

		if (std::filesystem::is_directory(path))
			if (path.back() != '/')
				path.push_back('/');

		return path;
	}

	std::pair<std::string, std::string> path_relative_folder(std::string filepath, std::string start = "") {
		std::string file_relative_path = ::path_normalize(std::filesystem::relative(filepath).string());
		rynx_assert(file_relative_path.starts_with(start), "filepath did not begin with indicated start");
		auto start_pos = file_relative_path.find(start);
		start = ::path_normalize(start);
		if (!start.empty()) {
			if (start.back() == '/') {
				start.pop_back();
			}
			file_relative_path = file_relative_path.substr(start_pos + start.length());
		}

		auto last_folder_separator = file_relative_path.find_last_of('/');
		std::string file_folder = file_relative_path.substr(0, last_folder_separator + 1);
		std::string file_name = file_relative_path.substr(last_folder_separator + 1);
		return { file_folder, file_name };
	}
}

namespace rynx::filesystem {
	namespace native {
		namespace {
			template<bool files, bool directories>
			std::vector<std::string> enumerate_native_fs_contents(const std::string& path, rynx::filesystem::recursive recurse) {
				if (!directory_exists(path)) {
					return {};
				}

				std::vector<std::string> result;
				auto fs_entry_handler = [&result](auto entry) {
					if constexpr (directories) {
						if (entry.is_directory()) {
							auto [directorypath, directoryname] = path_relative_folder(entry.path().string());
							result.emplace_back(directorypath + directoryname);
						}
					}
					if constexpr (files) {
						if (entry.is_regular_file()) {
							auto [filepath, filename] = path_relative_folder(entry.path().string());
							result.emplace_back(filepath + filename);
						}
					}
				};

				if (recurse == recursive::yes) {
					for (auto entry : std::filesystem::recursive_directory_iterator(path)) {
						fs_entry_handler(entry);
					}
				}
				else {
					for (auto entry : std::filesystem::directory_iterator(path)) {
						fs_entry_handler(entry);
					}
				}
				return result;
			}
		}

		std::vector<std::string> enumerate(const std::string& path, rynx::filesystem::recursive recurse) {
			return enumerate_native_fs_contents<true, true>(path, recurse);
		}

		std::vector<std::string> enumerate_files(const std::string& native_path, recursive recurse) {
			return enumerate_native_fs_contents<true, false>(native_path, recurse);
		}

		std::vector<std::string> enumerate_directories(const std::string& native_path, recursive recurse) {
			return enumerate_native_fs_contents<false, true>(native_path, recurse);
		}

		bool file_exists(const std::string& path) {
			return std::filesystem::is_regular_file(path);
		}

		bool directory_exists(const std::string& path) {
			return std::filesystem::is_directory(path);
		}

		bool create_directory(const std::string& path) {
			return std::filesystem::create_directories(path);
		}

		bool delete_directory(const std::string& path) {
			if (directory_exists(path))
				return std::filesystem::remove(path.c_str());
			return false;
		}

		bool delete_file(const std::string& path) {
			if (file_exists(path))
				return std::filesystem::remove(path.c_str());
			return false;
		}

		std::string absolute_path(const std::string& path) {
			return path_normalize(std::filesystem::absolute(path).string());
		}

		std::string relative_path(const std::string& path) {
			return path_normalize(std::filesystem::relative(path).string());
		}
	}
}