// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "ps2mc_dir.h"
#include <algorithm>
#include <ctime>

std::string zeroTerminate(const std::string& s)
{
	size_t pos = s.find('\0');
	if (pos == std::string::npos)
	{
		return s;
	}
	return s.substr(0, pos);
}

PS2McTod unpackTod(const std::vector<uint8_t>& data)
{
	PS2McTod tod{};
	if (data.size() >= 8)
	{
		tod.sec = data[1];
		tod.min = data[2];
		tod.hour = data[3];
		tod.mday = data[4];
		tod.month = data[5];
		tod.year = data[6] | (data[7] << 8);
	}
	return tod;
}

std::vector<uint8_t> packTod(const PS2McTod& tod)
{
	std::vector<uint8_t> data(8, 0);
	data[1] = tod.sec;
	data[2] = tod.min;
	data[3] = tod.hour;
	data[4] = tod.mday;
	data[5] = tod.month;
	data[6] = tod.year & 0xFF;
	data[7] = (tod.year >> 8) & 0xFF;
	return data;
}

PS2McDirEntry unpackDirEntry(const std::vector<uint8_t>& data)
{
	PS2McDirEntry entry{};

	if (data.size() < PS2MC_DIRENT_LENGTH)
	{
		return entry;
	}

	entry.mode = data[0] | (data[1] << 8);
	entry.unused = data[2] | (data[3] << 8);
	entry.length = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);

	std::vector<uint8_t> created_data(data.begin() + 8, data.begin() + 16);
	entry.created = unpackTod(created_data);

	entry.cluster = data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24);
	entry.dirEntry = data[20] | (data[21] << 8) | (data[22] << 16) | (data[23] << 24);

	std::vector<uint8_t> modified_data(data.begin() + 24, data.begin() + 32);
	entry.modified = unpackTod(modified_data);

	entry.attr = data[32] | (data[33] << 8) | (data[34] << 16) | (data[35] << 24);

	std::string name(data.begin() + 64, data.begin() + PS2MC_DIRENT_LENGTH);
	entry.name = zeroTerminate(name);

	return entry;
}

std::vector<uint8_t> packDirEntry(const PS2McDirEntry& entry)
{
	std::vector<uint8_t> data(PS2MC_DIRENT_LENGTH, 0);

	data[0] = entry.mode & 0xFF;
	data[1] = (entry.mode >> 8) & 0xFF;
	data[2] = entry.unused & 0xFF;
	data[3] = (entry.unused >> 8) & 0xFF;
	data[4] = entry.length & 0xFF;
	data[5] = (entry.length >> 8) & 0xFF;
	data[6] = (entry.length >> 16) & 0xFF;
	data[7] = (entry.length >> 24) & 0xFF;

	auto created_data = packTod(entry.created);
	std::copy(created_data.begin(), created_data.end(), data.begin() + 8);

	data[16] = entry.cluster & 0xFF;
	data[17] = (entry.cluster >> 8) & 0xFF;
	data[18] = (entry.cluster >> 16) & 0xFF;
	data[19] = (entry.cluster >> 24) & 0xFF;
	data[20] = entry.dirEntry & 0xFF;
	data[21] = (entry.dirEntry >> 8) & 0xFF;
	data[22] = (entry.dirEntry >> 16) & 0xFF;
	data[23] = (entry.dirEntry >> 24) & 0xFF;

	auto modified_data = packTod(entry.modified);
	std::copy(modified_data.begin(), modified_data.end(), data.begin() + 24);

	data[32] = entry.attr & 0xFF;
	data[33] = (entry.attr >> 8) & 0xFF;
	data[34] = (entry.attr >> 16) & 0xFF;
	data[35] = (entry.attr >> 24) & 0xFF;

	size_t name_len = std::min(entry.name.size(), size_t(448));
	std::copy(entry.name.begin(), entry.name.begin() + name_len, data.begin() + 64);

	return data;
}

std::time_t todToTime(const PS2McTod& tod)
{
	std::tm timeinfo = {};
	timeinfo.tm_sec = tod.sec;
	timeinfo.tm_min = tod.min;
	timeinfo.tm_hour = tod.hour;
	timeinfo.tm_mday = tod.mday;
	timeinfo.tm_mon = tod.month - 1;
	timeinfo.tm_year = tod.year - 1900;
	return std::mktime(&timeinfo);
}

PS2McTod timeToTod(std::time_t time)
{
	std::tm timeinfo{};
#if defined(_WIN32)
	localtime_s(&timeinfo, &time);
#else
	localtime_r(&time, &timeinfo);
#endif
	PS2McTod tod{};
	tod.sec = static_cast<uint8_t>(timeinfo.tm_sec);
	tod.min = static_cast<uint8_t>(timeinfo.tm_min);
	tod.hour = static_cast<uint8_t>(timeinfo.tm_hour);
	tod.mday = static_cast<uint8_t>(timeinfo.tm_mday);
	tod.month = static_cast<uint8_t>(timeinfo.tm_mon + 1);
	tod.year = static_cast<uint16_t>(timeinfo.tm_year + 1900);
	return tod;
}
