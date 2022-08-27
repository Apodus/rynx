
#pragma once

#include <span>
#include <vector>

std::vector<char> decompress(std::span<char> memory);
std::vector<char> compress(std::span<char> memory);