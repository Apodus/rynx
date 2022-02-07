#pragma once

#include "rynx/filesystem/filekinds/file.hpp"
#include <vector>

namespace rynx::filesystem {
	struct memory_file {
		std::vector<uint8_t> filecontent;

		bool empty() const { return filecontent.empty(); }
		size_t size() const { return filecontent.size(); }

		auto* data() { return filecontent.data(); }
		const auto* data() const { return filecontent.data(); }

		uint8_t* begin() { return filecontent.data(); }
		uint8_t* end() { return begin() + size(); }
		const uint8_t* end() const { return begin() + size(); }
		const uint8_t* begin() const { return filecontent.data(); }
	};

	class memoryfile_read : public iread_file {
	private:
		const memory_file& m_file;
		size_t m_offset = 0;
	public:
		memoryfile_read(const memory_file& m_file) : m_file(m_file) {}
		const auto begin() const { return m_file.filecontent.begin(); }
		const auto end() const { return m_file.filecontent.end(); }

		virtual void seek_beg(int64_t seek_to) override { m_offset = seek_to; };
		virtual void seek_cur(int64_t seek_to) override { m_offset += seek_to; };
		virtual size_t tell() const override { return m_offset; };
		virtual size_t size() const override { return m_file.filecontent.size(); };
		virtual size_t read(void* dst, size_t bytes) override;
	};

	class memoryfile_write : public iwrite_file {
	private:
		memory_file& m_file;
		size_t m_offset;
	public:
		memoryfile_write(memory_file& m_file, iwrite_file::mode mode = iwrite_file::mode::Overwrite) : m_file(m_file), m_offset(0) {
			if (mode == iwrite_file::mode::Append) {
				m_offset = m_file.size();
			}
		}
		virtual void write(const void* filecontent, size_t bytes) override;
	};
}