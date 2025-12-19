// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "sjis.h"
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#endif

// Shift-JIS normalization table
const std::unordered_map<char32_t, std::u32string>& ShiftJIS::getNormalizationTable()
{
	// clang-format off
	static const std::unordered_map<char32_t, std::u32string> table = {
		{0xFF81, U"\u30C1"}, {0x3000, U" "}, {0xFF85, U"\u30CA"}, {0xFF06, U"&"},
		{0xFF89, U"\u30CE"}, {0xFF0A, U"*"}, {0xFF8D, U"\u30D8"}, {0xFF0E, U"."},
		{0xFF91, U"\u30E0"}, {0xFF12, U"2"}, {0xFF95, U"\u30E6"}, {0xFF16, U"6"},
		{0xFF99, U"\u30EB"}, {0x309B, U" \u3099"}, {0xFF1A, U":"}, {0xFF9D, U"\u30F3"},
		{0xFF03, U"#"}, {0xFF1E, U">"}, {0xFF22, U"B"}, {0xFF26, U"F"},
		{0xFF2A, U"J"}, {0x222C, U"\u222B\u222B"}, {0xFF2E, U"N"}, {0xFF32, U"R"},
		{0xFF36, U"V"}, {0xFF3A, U"Z"}, {0xFF3E, U"^"}, {0xFF42, U"b"},
		{0xFF46, U"f"}, {0xFF4A, U"j"}, {0xFF4E, U"n"}, {0xFF52, U"r"},
		{0xFF56, U"v"}, {0xFF5A, U"z"}, {0xFF62, U"\u300C"}, {0xFFE5, U"\xA5"},
		{0xFF66, U"\u30F2"}, {0xFF6A, U"\u30A7"}, {0xFF6E, U"\u30E7"}, {0xFF72, U"\u30A4"},
		{0xFF76, U"\u30AB"}, {0xFF7A, U"\u30B3"}, {0xFF7E, U"\u30BB"}, {0xFF01, U"!"},
		{0xFF82, U"\u30C4"}, {0xFF05, U"%"}, {0xFF86, U"\u30CB"}, {0xFF09, U")"},
		{0xFF8A, U"\u30CF"}, {0xFF8E, U"\u30DB"}, {0xFF11, U"1"}, {0xFF92, U"\u30E1"},
		{0xFF15, U"5"}, {0xFF96, U"\u30E8"}, {0xFF19, U"9"}, {0xFF9A, U"\u30EC"},
		{0xFF1D, U"="}, {0x309C, U" \u309A"}, {0xFF9E, U"\u3099"}, {0xFF21, U"A"},
		{0xFF25, U"E"}, {0xFF29, U"I"}, {0x00A8, U" \u0308"}, {0xFF2D, U"M"},
		{0xFF31, U"Q"}, {0x2033, U"\u2032\u2032"}, {0xFF35, U"U"}, {0x00B4, U" \u0301"},
		{0xFF39, U"Y"}, {0xFF3D, U"]"}, {0xFF41, U"a"}, {0xFF45, U"e"},
		{0xFF49, U"i"}, {0xFF4D, U"m"}, {0xFF51, U"q"}, {0xFF55, U"u"},
		{0xFF59, U"y"}, {0xFF5D, U"}"}, {0xFF61, U"\u3002"}, {0xFF65, U"\u30FB"},
		{0xFF69, U"\u30A5"}, {0xFF6D, U"\u30E5"}, {0xFF71, U"\u30A2"}, {0xFF75, U"\u30AA"},
		{0xFF79, U"\u30B1"}, {0xFF7D, U"\u30B9"}, {0xFF83, U"\u30C6"}, {0xFF04, U"$"},
		{0xFF87, U"\u30CC"}, {0xFF08, U"("}, {0xFF8B, U"\u30D2"}, {0xFF0C, U","},
		{0xFF8F, U"\u30DE"}, {0xFF10, U"0"}, {0xFF93, U"\u30E2"}, {0xFF14, U"4"},
		{0xFF97, U"\u30E9"}, {0xFF18, U"8"}, {0xFF9B, U"\u30ED"}, {0xFF1C, U"<"},
		{0xFF9F, U"\u309A"}, {0xFF20, U"@"}, {0xFF24, U"D"}, {0x2026, U"..."},
		{0xFF28, U"H"}, {0xFF2C, U"L"}, {0xFF30, U"P"}, {0xFF34, U"T"},
		{0xFF38, U"X"}, {0xFF3C, U"\\"}, {0xFF40, U"`"}, {0xFF44, U"d"},
		{0xFF48, U"h"}, {0xFF4C, U"l"}, {0xFF50, U"p"}, {0xFF54, U"t"},
		{0xFF58, U"x"}, {0xFF5C, U"|"}, {0xFFE3, U" \u0304"}, {0xFF64, U"\u3001"},
		{0xFF68, U"\u30A3"}, {0xFF6C, U"\u30E3"}, {0xFF70, U"\u30FC"}, {0xFF74, U"\u30A8"},
		{0xFF78, U"\u30AF"}, {0xFF7C, U"\u30B7"}, {0xFF80, U"\u30BF"}, {0x2103, U"\xB0C"},
		{0xFF84, U"\u30C8"}, {0xFF88, U"\u30CD"}, {0xFF0B, U"+"}, {0xFF8C, U"\u30D5"},
		{0xFF0F, U"/"}, {0xFF90, U"\u30DF"}, {0xFF13, U"3"}, {0xFF94, U"\u30E4"},
		{0xFF17, U"7"}, {0xFF98, U"\u30EA"}, {0xFF1B, U";"}, {0xFF9C, U"\u30EF"},
		{0xFF1F, U"?"}, {0xFF23, U"C"}, {0x2025, U".."}, {0xFF27, U"G"},
		{0x212B, U"\xC5"}, {0xFF2F, U"O"}, {0xFF33, U"S"}, {0xFF37, U"W"},
		{0xFF3B, U"["}, {0xFF3F, U"_"}, {0xFF43, U"c"}, {0xFF47, U"g"},
		{0xFF4B, U"k"}, {0xFF4F, U"o"}, {0xFF53, U"s"}, {0xFF57, U"w"},
		{0xFF5B, U"{"}, {0xFF63, U"\u300D"}, {0xFF67, U"\u30A1"}, {0xFF6B, U"\u30A9"},
		{0xFF6F, U"\u30C3"}, {0xFF73, U"\u30A6"}, {0xFF77, U"\u30AD"}, {0xFF7B, U"\u30B5"},
		{0xFF2B, U"K"}, {0xFF7F, U"\u30BD"}};
	// clang-format on
	return table;
}

// Convert UTF-32 to UTF-8
static std::string utf32ToUtf8(const std::u32string& utf32)
{
	std::string result;
	for (char32_t codepoint : utf32)
	{
		if (codepoint <= 0x7F)
		{
			result += static_cast<char>(codepoint);
		}
		else if (codepoint <= 0x7FF)
		{
			result += static_cast<char>(0xC0 | (codepoint >> 6));
			result += static_cast<char>(0x80 | (codepoint & 0x3F));
		}
		else if (codepoint <= 0xFFFF)
		{
			result += static_cast<char>(0xE0 | (codepoint >> 12));
			result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
			result += static_cast<char>(0x80 | (codepoint & 0x3F));
		}
		else if (codepoint <= 0x10FFFF)
		{
			result += static_cast<char>(0xF0 | (codepoint >> 18));
			result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
			result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
			result += static_cast<char>(0x80 | (codepoint & 0x3F));
		}
	}
	return result;
}

// Convert UTF-8 to UTF-32
static std::u32string utf8ToUtf32(const std::string& utf8)
{
	std::u32string result;
	for (size_t i = 0; i < utf8.size();)
	{
		uint8_t byte = static_cast<uint8_t>(utf8[i]);
		char32_t codepoint = 0;

		if (byte < 0x80)
		{
			codepoint = byte;
			++i;
		}
		else if ((byte & 0xE0) == 0xC0 && i + 1 < utf8.size())
		{
			codepoint = ((byte & 0x1F) << 6) |
			            (static_cast<uint8_t>(utf8[i + 1]) & 0x3F);
			i += 2;
		}
		else if ((byte & 0xF0) == 0xE0 && i + 2 < utf8.size())
		{
			codepoint = ((byte & 0x0F) << 12) |
			            ((static_cast<uint8_t>(utf8[i + 1]) & 0x3F) << 6) |
			            (static_cast<uint8_t>(utf8[i + 2]) & 0x3F);
			i += 3;
		}
		else if ((byte & 0xF8) == 0xF0 && i + 3 < utf8.size())
		{
			codepoint = ((byte & 0x07) << 18) |
			            ((static_cast<uint8_t>(utf8[i + 1]) & 0x3F) << 12) |
			            ((static_cast<uint8_t>(utf8[i + 2]) & 0x3F) << 6) |
			            (static_cast<uint8_t>(utf8[i + 3]) & 0x3F);
			i += 4;
		}
		else
		{
			++i;
			continue;
		}

		result += codepoint;
	}
	return result;
}

// Normalize a UTF-8 string using the Shift-JIS normalization table
std::string ShiftJIS::normalize(const std::string& utf8)
{
	const auto& table = getNormalizationTable();
	std::u32string utf32 = utf8ToUtf32(utf8);
	std::u32string normalized;

	for (char32_t ch : utf32)
	{
		auto it = table.find(ch);
		if (it != table.end())
		{
			// Character has a normalization, append replacement
			normalized += it->second;
		}
		else
		{
			// No normalization, keep original
			normalized += ch;
		}
	}

	return utf32ToUtf8(normalized);
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
