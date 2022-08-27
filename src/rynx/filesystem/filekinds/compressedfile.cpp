
#include <rynx/filesystem/filekinds/compressedfile.hpp>
#include <rynx/std/compression.hpp>

rynx::filesystem::compressedfile_read::compressedfile_read(rynx::shared_ptr<rynx::filesystem::iread_file> backing_file) : m_decompressed_output(m_decompressed_file) {
	auto compressed = backing_file->read_all();
	auto result = decompress(compressed);
	m_decompressed_file.filecontent = std::move(reinterpret_cast<std::vector<uint8_t>&>(result));
}

rynx::filesystem::compressedfile_write::~compressedfile_write() {
	auto compressed = compress({ reinterpret_cast<char*>(m_temp_file.data()), m_temp_file.size() });
	m_backing_file->write(compressed.data(), compressed.size());
}

void rynx::filesystem::compressedfile_write::write(const void* filecontent, size_t bytes) {
	m_temp_output.write(filecontent, bytes);
}