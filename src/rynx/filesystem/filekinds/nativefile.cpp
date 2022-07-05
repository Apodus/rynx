
#include "rynx/filesystem/filekinds/nativefile.hpp"
#include <fstream>

namespace {
	std::string to_std(const rynx::string& str) {
		return std::string(str.data(), str.size());
	}

	rynx::string to_rynx(const std::string& str) {
		return rynx::string(str.data(), str.size());
	}
}

namespace rynx::filesystem {
	nativefile_read::nativefile_read(const rynx::string& path) {
		open(path);
		auto beginPos = m_nativeFile->tellg();
		m_nativeFile->seekg(0, std::ios::end);
		m_size = m_nativeFile->tellg() - beginPos;
		seek_beg(0);
	}

	nativefile_read::nativefile_read(nativefile_read&& other) noexcept { *this = std::move(other); }

	nativefile_read& nativefile_read::operator=(nativefile_read&& other) noexcept {
		m_nativeFile = std::move(other.m_nativeFile);
		m_size = other.m_size;
		m_offset = other.m_offset;
		other.m_size = 0;
		other.m_offset = 0;
		return *this;
	}

	nativefile_read::~nativefile_read() {
		close();
	}

	void nativefile_read::seek_beg(int64_t pos) {
		m_offset = pos;
		m_nativeFile->seekg(pos, std::ios::beg);
	}
	void nativefile_read::seek_cur(int64_t pos) {
		m_offset += pos;
		m_nativeFile->seekg(pos, std::ios::cur);
	}
	size_t nativefile_read::tell() const { return m_offset; }
	size_t nativefile_read::size() const { return m_size; }

	void nativefile_read::open(const rynx::string& path) {
		m_nativeFile = rynx::make_opaque_unique_ptr<std::ifstream>();
		m_nativeFile->open(to_std(path), std::ios_base::binary);
	}

	size_t nativefile_read::read(void* dst, size_t bytes) {
		m_nativeFile->read(static_cast<char*>(dst), bytes);
		m_offset += bytes;
		return bytes; // this is not exactly true
	}

	void nativefile_read::close() {
		m_nativeFile->close();
	}

	nativefile_write::nativefile_write(const rynx::string& path, filesystem::iwrite_file::mode mode) {
		open(path, mode);
	}

	nativefile_write::nativefile_write(nativefile_write&& source)
		: m_nativeFile(std::move(source.m_nativeFile)) {
	}

	nativefile_write::~nativefile_write() {
		close();
	}

	void nativefile_write::open(const rynx::string& path, iwrite_file::mode mode) {
		auto pos = path.find_last_of("/\\");
		if (pos != rynx::string::npos) {
			auto directoryPath = path.substr(0, pos);
			if (!rynx::filesystem::native::directory_exists(directoryPath)) {
				rynx::filesystem::native::create_directory(directoryPath);
			}
		}
		
		m_nativeFile = rynx::make_opaque_unique_ptr<std::ofstream>();
		if(mode == iwrite_file::mode::Append)
			m_nativeFile->open(path.c_str(), std::ios_base::app | std::ios_base::binary);
		else if(mode == iwrite_file::mode::Overwrite)
			m_nativeFile->open(path.c_str(), std::ios_base::binary);
	}

	void nativefile_write::close() {
		m_nativeFile->close();
	}

	void nativefile_write::write(const void* filecontent, size_t bytes) {
		m_nativeFile->write(static_cast<const char*>(filecontent), bytes);
	}
}