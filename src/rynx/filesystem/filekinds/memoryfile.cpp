
#include <rynx/filesystem/filekinds/memoryfile.hpp>
#include <rynx/system/assert.hpp>

size_t rynx::filesystem::memoryfile_read::read(void* dst, size_t bytes) {
	rynx_assert(m_offset + bytes <= m_file.filecontent.size(), "read out of bounds");
	memcpy(dst, &(m_file.filecontent[m_offset]), bytes);
	m_offset += bytes;
	return bytes;
};

void rynx::filesystem::memoryfile_write::write(const void* filecontent, size_t bytes) {
	if (bytes == 0)
		return;

	auto& fileData = m_file.filecontent;
	auto spaceNeeded = m_offset + bytes;
	if (spaceNeeded > fileData.size()) {
		if (spaceNeeded > fileData.capacity()) {
			fileData.reserve(2 * spaceNeeded);
		}
		fileData.resize(spaceNeeded);
	}

	memcpy(&fileData[m_offset], filecontent, bytes);
	m_offset += bytes;
}