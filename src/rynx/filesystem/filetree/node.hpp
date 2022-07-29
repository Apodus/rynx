
#pragma once

#include <rynx/filesystem/filekinds/file.hpp>
#include <rynx/filesystem/filekinds/memoryfile.hpp>
#include <rynx/filesystem/filekinds/nativefile.hpp>
#include <rynx/filesystem/native_fs.hpp>
#include <rynx/std/unordered_map.hpp>
#include <rynx/std/memory.hpp>
#include <rynx/std/string.hpp>

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
			node(const rynx::string& name);

			virtual rynx::shared_ptr<rynx::filesystem::iread_file> open_read(const rynx::string&);
			virtual rynx::shared_ptr<rynx::filesystem::iwrite_file> open_write(
				const rynx::string&, // path
				rynx::filesystem::iwrite_file::mode);
			virtual bool remove(const rynx::string&);

			rynx::string full_vfs_path() const;
			bool is_file_mount() const;

			virtual bool file_exists(const rynx::string&) const;
			virtual bool directory_exists(const rynx::string&) const;
			bool exists(const rynx::string& path) const;

			virtual std::vector<rynx::string> enumerate_content(
				const rynx::string&,
				rynx::filesystem::recursive,
				rynx::filesystem::filetree::detail::enumerate_flags);

			rynx::string name() const { return m_name; }
			rynx::weak_ptr<node> parent() const { return m_parent; }

			rynx::string m_name;
			rynx::weak_ptr<node> m_parent;
			rynx::unordered_map<rynx::string, rynx::shared_ptr<node>> m_children;
		};
	}
}