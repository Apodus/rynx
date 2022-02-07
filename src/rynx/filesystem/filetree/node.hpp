
#pragma once

#include "rynx/filesystem/filekinds/file.hpp"
#include "rynx/filesystem/filekinds/memoryfile.hpp"
#include "rynx/filesystem/filekinds/nativefile.hpp"
#include "rynx/filesystem/native_fs.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <algorithm>

namespace rynx::filesystem {
	class vfs;
	namespace filetree {
		namespace detail {
			struct enumerate_flags {

				enum flag {
					flag_files = 1,
					flag_directories = 2
				};

				bool files() const { return flags & flag::flag_files; }
				bool directories() const { return flags & flag::flag_directories; }

				int32_t flags = 0;
			};
		}

		class node {
		public:
			node(const std::string& name);

			virtual std::shared_ptr<rynx::filesystem::iread_file> open_read(const std::string&);
			virtual std::shared_ptr<rynx::filesystem::iwrite_file> open_write(
				const std::string&, // path
				rynx::filesystem::iwrite_file::mode);
			virtual bool remove(const std::string&);

			std::string full_vfs_path() const;
			bool is_file_mount() const;

			virtual bool file_exists(const std::string&) const;
			virtual bool directory_exists(const std::string&) const;
			bool exists(const std::string& path) const;

			virtual std::vector<std::string> enumerate_content(
				const std::string&,
				rynx::filesystem::recursive,
				rynx::filesystem::filetree::detail::enumerate_flags);

			std::string name() const { return m_name; }
			std::weak_ptr<node> parent() const { return m_parent; }

			std::string m_name;
			std::weak_ptr<node> m_parent;
			std::unordered_map<std::string, std::shared_ptr<node>> m_children;
		};

		class memoryfile_node : public node {
		public:
			memoryfile_node(const std::string& name) : node(name) {}
			memoryfile_node(const std::string& name, rynx::filesystem::memory_file file) : node(name) {
				m_file = std::move(file);
			}

			virtual bool file_exists(const std::string& path) const override {
				return path.empty();
			}

			virtual std::shared_ptr<rynx::filesystem::iwrite_file> open_write(const std::string& path, filesystem::iwrite_file::mode mode) override {
				if (path == "")
					return std::make_shared<memoryfile_write>(m_file, mode);
				return {};
			}

			virtual std::shared_ptr<rynx::filesystem::iread_file> open_read(const std::string& path) override {
				if (path.empty())
					return std::make_shared<rynx::filesystem::memoryfile_read>(m_file);
				return {};
			}

			virtual bool remove(const std::string& path) override {
				// TODO:
				if (path.empty()) {

				}
				return false;
			}

			virtual std::vector<std::string> enumerate_content(
				const std::string& path,
				rynx::filesystem::recursive,
				rynx::filesystem::filetree::detail::enumerate_flags flags) override {
				if (path.empty() && flags.files())
					return { name() };
				return {};
			}
		private:
			rynx::filesystem::memory_file m_file;
		};
	}
}