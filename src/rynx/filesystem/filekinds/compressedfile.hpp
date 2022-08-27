
#pragma once

#include <rynx/filesystem/filekinds/file.hpp>
#include <rynx/filesystem/filekinds/memoryfile.hpp>
#include <rynx/std/memory.hpp>

namespace rynx::filesystem {
	class compressedfile_read : public iread_file {
		rynx::filesystem::memory_file m_decompressed_file;
		rynx::filesystem::memoryfile_read m_decompressed_output;

	public:
		compressedfile_read(rynx::shared_ptr<rynx::filesystem::iread_file> backing_file);
		virtual ~compressedfile_read() {}

		const auto begin() const { return m_decompressed_output.begin(); }
		const auto end() const { return m_decompressed_output.end(); }

		virtual void seek_beg(int64_t seek_to) override { m_decompressed_output.seek_beg(seek_to); };
		virtual void seek_cur(int64_t seek_to) override { m_decompressed_output.seek_cur(seek_to); };
		virtual size_t tell() const override { return m_decompressed_output.tell(); };
		virtual size_t size() const override { return m_decompressed_output.size(); };
		virtual size_t read(void* dst, size_t bytes) override { return m_decompressed_output.read(dst, bytes); }
	};

	class compressedfile_write : public iwrite_file {
		rynx::filesystem::memory_file m_temp_file;
		rynx::filesystem::memoryfile_write m_temp_output;
		rynx::shared_ptr<rynx::filesystem::iwrite_file> m_backing_file;

	public:
		compressedfile_write(rynx::shared_ptr<rynx::filesystem::iwrite_file> backing_file) :
			m_backing_file(std::move(backing_file)),
			m_temp_output(m_temp_file)
		{}
		
		virtual ~compressedfile_write();
		virtual void write(const void* filecontent, size_t bytes) override;
	};
}
