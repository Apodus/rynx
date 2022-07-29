#pragma once

#include <rynx/filesystem/filetree/node.hpp>
#include <rynx/filesystem/filekinds/file.hpp>
#include <rynx/std/string.hpp>

#ifndef FileSystemDLL
#define FileSystemDLL
#endif

namespace rynx {
	namespace filesystem {
		class FileSystemDLL vfs {
		public:
			class vfs_directory_handle {
				vfs& m_host;
				rynx::string m_virtual_directory_path;
			public:
				vfs_directory_handle(vfs& m_host, rynx::string directory_virtual_path) : m_host(m_host), m_virtual_directory_path(std::move(directory_virtual_path)) {}
				rynx::shared_ptr<rynx::filesystem::iwrite_file> open_write(rynx::string path) { return m_host.open_write(m_virtual_directory_path + path); }
				rynx::shared_ptr<rynx::filesystem::iread_file> open_read(rynx::string path) { return m_host.open_read(m_virtual_directory_path + path); }
			};

			class mounter {
				vfs& m_host;
			public:
				mounter(vfs& host) : m_host(host) {}
				void memory_file(rynx::string virtual_path) { m_host.memory_file(std::move(virtual_path)); }
				void memory_directory(rynx::string virtual_path) { m_host.memory_directory(std::move(virtual_path)); }
				void native_directory(rynx::string native_path, rynx::string virtual_path) { m_host.native_directory(std::move(native_path), std::move(virtual_path)); }
				void native_file(rynx::string native_path, rynx::string virtual_path) { m_host.native_file(std::move(native_path), std::move(virtual_path)); }
			};

			vfs();
			~vfs();

			vfs(const vfs& other) = delete;
			vfs(vfs&& other) = delete;
			void operator = (vfs&& other) = delete;
			void operator = (const vfs& other) = delete;

			vfs_directory_handle operator()(rynx::string virtual_directory_path) {
				if (virtual_directory_path.back() != '/')
					virtual_directory_path.push_back('/');
				return vfs_directory_handle(*this, std::move(virtual_directory_path));
			}

			bool file_exists(rynx::string path) const;
			bool directory_exists(rynx::string path) const;
			
			rynx::shared_ptr<rynx::filesystem::iwrite_file> open_write(rynx::string virtual_path, filesystem::iwrite_file::mode mode = filesystem::iwrite_file::mode::Overwrite) const;
			rynx::shared_ptr<rynx::filesystem::iread_file> open_read(rynx::string virtual_path) const;
			bool remove(rynx::string virtual_path) const;
			
			std::vector<rynx::string> enumerate(rynx::string virtual_path, rynx::filesystem::recursive recurse = rynx::filesystem::recursive::no) const;
			std::vector<rynx::string> enumerate_files(rynx::string virtual_path, rynx::filesystem::recursive recurse = rynx::filesystem::recursive::no) const;
			std::vector<rynx::string> enumerate_directories(rynx::string virtual_path, rynx::filesystem::recursive recurse = rynx::filesystem::recursive::no) const;

			mounter mount() { return mounter{ *this }; }
			void unmount(rynx::string virtual_path);

		private:
			void memory_file(rynx::string virtual_path);
			void memory_directory(rynx::string virtual_path);
			void native_directory(rynx::string native_path, rynx::string virtual_path);
			void native_file(rynx::string native_path, rynx::string virtual_path);

			std::vector<rynx::string> enumerate_vfs_content(rynx::string virtual_path, rynx::filesystem::recursive recurse, filetree::detail::enumerate_flags) const;

			rynx::shared_ptr<rynx::filesystem::filetree::node> vfs_tree_mount(rynx::string virtual_path) const;
			std::pair<rynx::shared_ptr<rynx::filesystem::filetree::node>, rynx::string> vfs_tree_access(rynx::string virtual_path) const;

			rynx::shared_ptr<rynx::filesystem::filetree::node> m_filetree_root;
		};
	}
}
