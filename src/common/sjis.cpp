// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "sjis.h"

#include <utf8proc.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>

#endif

// Normalize a UTF-8 string using NFKC via utf8proc
std::string ShiftJIS::normalize(const std::string& utf8)
{
	if (utf8.empty())
		return utf8;

	utf8proc_uint8_t* result = nullptr;
	utf8proc_ssize_t len = utf8proc_map(
		reinterpret_cast<const utf8proc_uint8_t*>(utf8.data()),
		static_cast<utf8proc_ssize_t>(utf8.size()),
		&result,
		static_cast<utf8proc_option_t>(
			UTF8PROC_STABLE | UTF8PROC_COMPOSE | UTF8PROC_COMPAT));

	if (len < 0 || !result)
		return utf8;

	std::string normalized(reinterpret_cast<const char*>(result), static_cast<size_t>(len));
	free(result);
	return normalized;
}

std::string ShiftJIS::toUtf8(const std::string& sjis, bool normalizeResult)
{
#ifdef _WIN32
	if (sjis.empty())
		return "";

	// Convert Shift-JIS to UTF-16
	int wideLen = MultiByteToWideChar(932, 0, sjis.c_str(), static_cast<int>(sjis.size()), nullptr, 0);
	if (wideLen == 0)
		return "";

	std::wstring wideStr(wideLen, 0);
	MultiByteToWideChar(932, 0, sjis.c_str(), static_cast<int>(sjis.size()), &wideStr[0], wideLen);

	// Convert UTF-16 to UTF-8
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), static_cast<int>(wideStr.size()), nullptr, 0, nullptr, nullptr);
	if (utf8Len == 0)
		return "";

	std::string result(utf8Len, 0);
	WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), static_cast<int>(wideStr.size()), &result[0], utf8Len, nullptr, nullptr);

	// Apply normalization if requested
	if (normalizeResult)
	{
		result = normalize(result);
	}

	return result;
#else
	// Use iconv for POSIX systems (Linux/macOS)
	if (sjis.empty())
		return "";

	iconv_t cd = iconv_open("UTF-8", "SHIFT_JIS");
	if (cd == reinterpret_cast<iconv_t>(-1))
	{
		// iconv not available or encoding not supported, return as is
		return sjis;
	}

	size_t inBytesLeft = sjis.size();
	size_t outBytesLeft = sjis.size() * 4 + 4; // UTF-8 can expand
	std::string result(outBytesLeft, '\0');

	char* inBuf = const_cast<char*>(sjis.data());
	char* outBuf = &result[0];
	char* outStart = outBuf;

	size_t convResult = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
	iconv_close(cd);

	if (convResult == static_cast<size_t>(-1))
	{
		// Conversion error, return input as is
		return sjis;
	}

	result.resize(outBuf - outStart);

	// Apply normalization if requested
	if (normalizeResult)
	{
		result = normalize(result);
	}

	return result;
#endif
}

std::string ShiftJIS::fromUtf8(const std::string& utf8)
{
#ifdef _WIN32
	if (utf8.empty())
		return "";

	// Convert UTF-8 to UTF-16
	int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
	if (wideLen == 0)
		return "";

	std::wstring wideStr(wideLen, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), &wideStr[0], wideLen);

	// Convert UTF-16 to Shift-JIS (codepage 932)
	int sjisLen = WideCharToMultiByte(932, 0, wideStr.c_str(), static_cast<int>(wideStr.size()), nullptr, 0, nullptr, nullptr);
	if (sjisLen == 0)
		return "";

	std::string result(sjisLen, 0);
	WideCharToMultiByte(932, 0, wideStr.c_str(), static_cast<int>(wideStr.size()), &result[0], sjisLen, nullptr, nullptr);

	return result;
#else
	// Use iconv for POSIX systems (Linux/macOS)
	if (utf8.empty())
		return "";

	iconv_t cd = iconv_open("SHIFT_JIS", "UTF-8");
	if (cd == reinterpret_cast<iconv_t>(-1))
	{
		// iconv not available or encoding not supported, return as is
		return utf8;
	}

	size_t inBytesLeft = utf8.size();
	size_t outBytesLeft = utf8.size() * 2 + 2; // SJIS is at most 2 bytes per char
	std::string result(outBytesLeft, '\0');

	char* inBuf = const_cast<char*>(utf8.data());
	char* outBuf = &result[0];
	char* outStart = outBuf;

	size_t convResult = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft);
	iconv_close(cd);

	if (convResult == static_cast<size_t>(-1))
	{
		// Conversion error, return input as is
		return utf8;
	}

	result.resize(outBuf - outStart);
	return result;
#endif
}

bool ShiftJIS::isLeadByte(uint8_t byte)
{
	return (byte >= 0x81 && byte <= 0x9F) || (byte >= 0xE0 && byte <= 0xFC);
}

int ShiftJIS::charLength(const uint8_t* str)
{
	if (!str)
		return 0;

	uint8_t byte = *str;

	if (byte == 0)
		return 0;
	if (byte < 0x80)
		return 1; // ASCII
	if ((byte >= 0x81 && byte <= 0x9F) || (byte >= 0xE0 && byte <= 0xFC))
	{
		return 2; // Double-byte
	}
	if (byte >= 0xA0 && byte <= 0xDF)
		return 1;

	return 1;
}
