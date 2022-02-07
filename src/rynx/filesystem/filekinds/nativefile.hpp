#pragma once

#include "rynx/filesystem/filekinds/file.hpp"
#include "rynx/filesystem/native_fs.hpp"

#include <fstream>

namespace rynx {
	namespace filesystem {
		class nativefile_read : public iread_file {
			nativefile_read(const nativefile_read&) = delete;
			nativefile_read& operator=(const nativefile_read&) = delete;

			size_t m_offset = 0;
			size_t m_size = 0;
			
			std::shared_ptr<uint8_t> m_blob;
			std::ifstream m_nativeFile;

			void open(const std::string& path);
			void close();

		public:
			virtual void seek_beg(int64_t pos) override;
			virtual void seek_cur(int64_t pos) override;
			virtual size_t tell() const override;
			virtual size_t size() const override;
			virtual size_t read(void* dst, size_t bytes) override;

			nativefile_read(const std::string& path);
			~nativefile_read();

			nativefile_read(nativefile_read&& other) noexcept { *this = std::move(other); }
			nativefile_read& operator=(nativefile_read&& other) noexcept {
				m_nativeFile = std::move(other.m_nativeFile);
				m_size = other.m_size;
				m_offset = other.m_offset;
				other.m_size = 0;
				other.m_offset = 0;
				return *this;
			}
		};

		class nativefile_write : public iwrite_file {
		private:

			nativefile_write(const nativefile_write&) = delete;
			nativefile_write& operator=(const nativefile_write&) = delete;
			nativefile_write& operator=(nativefile_write&&) = delete;

			std::ofstream m_nativeFile;

			void open(const std::string& path, iwrite_file::mode mode);
			void close();

		public:
			nativefile_write(const std::string& path, filesystem::iwrite_file::mode mode);
			nativefile_write(nativefile_write&& source);
			~nativefile_write();

			virtual void write(const void* filecontent, size_t bytes) override;
		};
	}
}