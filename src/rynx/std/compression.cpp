
#include <rynx/std/compression.hpp>
#include <rynx/system/assert.hpp>
#include <zstd.h>

// size_t ZSTD_compress(void* dst, size_t dstCapacity, const void* src, size_t srcSize, int compressionLevel);
// size_t ZSTD_decompress(void* dst, size_t dstCapacity, const void* src, size_t compressedSize);
// ZSTD_getFrameContentSize(compressedBuffer, bufferSize);
// size_t ZSTD_compressBound(size_t srcSize); /*!< maximum compressed size in worst case single-pass scenario */

std::vector<char> compress(std::span<char> memory) {
	size_t target_size = ZSTD_compressBound(memory.size_bytes());
	std::vector<char> result;
	result.resize(target_size);
	size_t compressedSize = ZSTD_compress(result.data(), result.size(), memory.data(), memory.size(), ZSTD_maxCLevel());
	result.resize(compressedSize);
	return result;
}

std::vector<char> decompress(std::span<char> memory) {
	if (memory.empty())
		return {};
	size_t expected_decompressed_size = ZSTD_getFrameContentSize(memory.data(), memory.size());
	std::vector<char> result(expected_decompressed_size);
	size_t actual_decompressed_size = ZSTD_decompress(result.data(), result.size(), memory.data(), memory.size());
	rynx_assert(actual_decompressed_size == expected_decompressed_size, "??");
	return result;
}