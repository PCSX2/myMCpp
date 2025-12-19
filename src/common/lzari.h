// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <vector>
#include <cstdint>
#include <memory>

// LZARI compression algorithm constants
const int HIST_LEN = 4096;
const int MIN_MATCH_LEN = 3;
const int MAX_MATCH_LEN = 60;

class LzariCodec
{
public:
	LzariCodec();
	~LzariCodec();

	// Compress data
	std::vector<uint8_t> compress(const std::vector<uint8_t>& input);

	// Decompress data
	std::vector<uint8_t> decompress(const std::vector<uint8_t>& input, size_t outputSize = 0);

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

std::vector<uint8_t> lzariCompress(const std::vector<uint8_t>& data);
std::vector<uint8_t> lzariDecompress(const std::vector<uint8_t>& data, size_t outputSize = 0);
