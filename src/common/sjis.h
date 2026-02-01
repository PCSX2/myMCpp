// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>

class ShiftJIS
{
public:
	static std::string toUtf8(const std::string& sjis, bool normalize = true);

	// Convert UTF-8 to Shift-JIS
	static std::string fromUtf8(const std::string& utf8);

	// Check if byte is Shift-JIS lead byte
	static bool isLeadByte(uint8_t byte);

	// Get Shift-JIS character length
	static int charLength(const uint8_t* str);

	// Normalize a UTF-8 string using the Shift-JIS normalization table
	static std::string normalize(const std::string& utf8);

private:
	ShiftJIS() = delete;

	// Shift-JIS normalization table
	static const std::unordered_map<char32_t, std::u32string>& getNormalizationTable();
};
