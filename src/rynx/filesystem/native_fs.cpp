
#include <rynx/filesystem/virtual_filesystem.hpp>
#include <rynx/system/assert.hpp>
#include <filesystem>

namespace {
	std::string to_std(const rynx::string& str) {
		return std::string(str.data(), str.size());
	}

	rynx::string to_rynx(const std::string& str) {
		return rynx::string(str.data(), str.size());
	}


	rynx::string path_normalize(rynx::string path) {
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

		if (std::filesystem::is_directory(to_std(path)))
			if (path.back() != '/')
				path.push_back('/');

		return path;
	}

	std::pair<rynx::string, rynx::string> path_relative_folder(rynx::string filepath, rynx::string start = "") {
		rynx::string file_relative_path = ::path_normalize(to_rynx(std::filesystem::relative(to_std(filepath)).string()));
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
		rynx::string file_folder = file_relative_path.substr(0, last_folder_separator + 1);
		rynx::string file_name = file_relative_path.substr(last_folder_separator + 1);
		return { file_folder, file_name };
	}
}

namespace rynx::filesystem {
	namespace native {
		namespace {
			template<bool files, bool directories>
			std::vector<rynx::string> enumerate_native_fs_contents(const rynx::string& path, rynx::filesystem::recursive recurse) {
				if (!directory_exists(path)) {
					return {};
				}

				std::vector<rynx::string> result;
				auto fs_entry_handler = [&result](auto entry) {
					if constexpr (directories) {
						if (entry.is_directory()) {
							auto [directorypath, directoryname] = path_relative_folder(to_rynx(entry.path().string()));
							result.emplace_back(directorypath + directoryname);
						}
					}
					if constexpr (files) {
						if (entry.is_regular_file()) {
							auto [filepath, filename] = path_relative_folder(to_rynx(entry.path().string()));
							result.emplace_back(filepath + filename);
						}
					}
				};

				if (recurse == recursive::yes) {
					for (auto entry : std::filesystem::recursive_directory_iterator(to_std(path))) {
						fs_entry_handler(entry);
					}
				}
				else {
					for (auto entry : std::filesystem::directory_iterator(to_std(path))) {
						fs_entry_handler(entry);
					}
				}
				return result;
			}
		}

		std::vector<rynx::string> enumerate(const rynx::string& path, rynx::filesystem::recursive recurse) {
			return enumerate_native_fs_contents<true, true>(path, recurse);
		}

		std::vector<rynx::string> enumerate_files(const rynx::string& native_path, recursive recurse) {
			return enumerate_native_fs_contents<true, false>(native_path, recurse);
		}

		std::vector<rynx::string> enumerate_directories(const rynx::string& native_path, recursive recurse) {
			return enumerate_native_fs_contents<false, true>(native_path, recurse);
		}

		bool file_exists(const rynx::string& path) {
			return std::filesystem::is_regular_file(path.c_str());
		}

		bool directory_exists(const rynx::string& path) {
			return std::filesystem::is_directory(path.c_str());
		}

		bool create_directory(const rynx::string& path) {
			return std::filesystem::create_directories(path.c_str());
		}

		bool delete_directory(const rynx::string& path) {
			if (directory_exists(path))
				return std::filesystem::remove(path.c_str());
			return false;
		}

		bool delete_file(const rynx::string& path) {
			if (file_exists(path))
				return std::filesystem::remove(path.c_str());
			return false;
		}

		rynx::string absolute_path(const rynx::string& path) {
			return path_normalize(to_rynx(std::filesystem::absolute(path.c_str()).string()));
		}

		rynx::string relative_path(const rynx::string& path) {
			return path_normalize(to_rynx(std::filesystem::relative(path.c_str()).string()));
		}
	}

	rynx::string resolve_path(rynx::string path) {
		while (true) {
			auto pos = path.find("/../");
			if (pos != rynx::string::npos) {
				auto pos2 = path.find_last_of('/', pos);
				path.replace(pos2, pos + 4, "");
			}
			else {
				return path;
			}
		}
	}
}