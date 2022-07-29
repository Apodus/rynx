#pragma once

#include "rynx/filesystem/filekinds/file.hpp"
#include "rynx/filesystem/native_fs.hpp"
#include <rynx/std/memory.hpp>
#include <iosfwd>

namespace rynx {
	namespace filesystem {
		class nativefile_read : public iread_file {
			nativefile_read(const nativefile_read&) = delete;
			nativefile_read& operator=(const nativefile_read&) = delete;

			size_t m_offset = 0;
			size_t m_size = 0;
			
			rynx::opaque_unique_ptr<std::ifstream> m_nativeFile;

			void open(const rynx::string& path);
			void close();

		public:
			virtual void seek_beg(int64_t pos) override;
			virtual void seek_cur(int64_t pos) override;
			virtual size_t tell() const override;
			virtual size_t size() const override;
			virtual size_t read(void* dst, size_t bytes) override;

			nativefile_read(const rynx::string& path);
			~nativefile_read();

			nativefile_read(nativefile_read&& other) noexcept;
			nativefile_read& operator=(nativefile_read&& other) noexcept;
		};

		class nativefile_write : public iwrite_file {
		private:

			nativefile_write(const nativefile_write&) = delete;
			nativefile_write& operator=(const nativefile_write&) = delete;
			nativefile_write& operator=(nativefile_write&&) = delete;

			rynx::opaque_unique_ptr<std::ofstream> m_nativeFile;

			void open(const rynx::string& path, iwrite_file::mode mode);
			void close();

		public:
			nativefile_write(const rynx::string& path, filesystem::iwrite_file::mode mode);
			nativefile_write(nativefile_write&& source);
			~nativefile_write();

			virtual void write(const void* filecontent, size_t bytes) override;
		};
	}
}