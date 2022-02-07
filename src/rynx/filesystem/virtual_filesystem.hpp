#pragma once

#include "rynx/filesystem/filetree/node.hpp"
#include "rynx/filesystem/filekinds/file.hpp"

#include <memory>
#include <string>

namespace rynx {
	namespace filesystem {
		class vfs {
		public:
			class vfs_directory_handle {
				vfs& m_host;
				std::string m_virtual_directory_path;
			public:
				vfs_directory_handle(vfs& m_host, std::string directory_virtual_path) : m_host(m_host), m_virtual_directory_path(std::move(directory_virtual_path)) {}
				std::shared_ptr<rynx::filesystem::iwrite_file> open_write(std::string path) { return m_host.open_write(m_virtual_directory_path + path); }
				std::shared_ptr<rynx::filesystem::iread_file> open_read(std::string path) { return m_host.open_read(m_virtual_directory_path + path); }
			};

			class mounter {
				vfs& m_host;
			public:
				mounter(vfs& host) : m_host(host) {}
				void memory_file(std::string virtual_path) { m_host.memory_file(std::move(virtual_path)); }
				void memory_directory(std::string virtual_path) { m_host.memory_directory(std::move(virtual_path)); }
				void native_directory(std::string native_path, std::string virtual_path) { m_host.native_directory(std::move(native_path), std::move(virtual_path)); }
				void native_file(std::string native_path, std::string virtual_path) { m_host.native_file(std::move(native_path), std::move(virtual_path)); }
			};

			vfs();
			~vfs();

			vfs(const vfs& other) = delete;
			vfs(vfs&& other) = delete;
			void operator = (vfs&& other) = delete;
			void operator = (const vfs& other) = delete;

			vfs_directory_handle operator()(std::string virtual_directory_path) {
				if (virtual_directory_path.back() != '/')
					virtual_directory_path.push_back('/');
				return vfs_directory_handle(*this, std::move(virtual_directory_path));
			}

			bool file_exists(std::string path) const;
			bool directory_exists(std::string path) const;
			
			std::shared_ptr<rynx::filesystem::iwrite_file> open_write(std::string virtual_path, filesystem::iwrite_file::mode mode = filesystem::iwrite_file::mode::Overwrite) const;
			std::shared_ptr<rynx::filesystem::iread_file> open_read(std::string virtual_path) const;
			bool remove(std::string virtual_path) const;
			
			std::vector<std::string> enumerate(std::string virtual_path, rynx::filesystem::recursive recurse = rynx::filesystem::recursive::no) const;
			std::vector<std::string> enumerate_files(std::string virtual_path, rynx::filesystem::recursive recurse = rynx::filesystem::recursive::no) const;
			std::vector<std::string> enumerate_directories(std::string virtual_path, rynx::filesystem::recursive recurse = rynx::filesystem::recursive::no) const;

			mounter mount() { return mounter{ *this }; }
			void unmount(std::string virtual_path);

		private:
			void memory_file(std::string virtual_path);
			void memory_directory(std::string virtual_path);
			void native_directory(std::string native_path, std::string virtual_path);
			void native_file(std::string native_path, std::string virtual_path);

			std::vector<std::string> enumerate_vfs_content(std::string virtual_path, rynx::filesystem::recursive recurse, filetree::detail::enumerate_flags) const;

			std::shared_ptr<rynx::filesystem::filetree::node> vfs_tree_mount(std::string virtual_path) const;
			std::pair<std::shared_ptr<rynx::filesystem::filetree::node>, std::string> vfs_tree_access(std::string virtual_path) const;

			std::shared_ptr<rynx::filesystem::filetree::node> m_filetree_root;
		};
	}
}
