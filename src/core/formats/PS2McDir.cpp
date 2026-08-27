// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+
// Directory entry packing/unpacking follows the mymc++ / mymc implementation and PS2 Browser directory structure docs.

#include "PS2McDir.h"
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
		tod.year = static_cast<uint16_t>(data[6] | (data[7] << 8));
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

	entry.mode = static_cast<uint16_t>(data[0] | (data[1] << 8));
	entry.unused = static_cast<uint16_t>(data[2] | (data[3] << 8));
	entry.length = static_cast<uint32_t>(data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24));

	std::vector<uint8_t> createdData(data.begin() + 8, data.begin() + 16);
	entry.created = unpackTod(createdData);

	entry.cluster = static_cast<uint32_t>(data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24));
	entry.dirEntry = static_cast<uint32_t>(data[20] | (data[21] << 8) | (data[22] << 16) | (data[23] << 24));

	std::vector<uint8_t> modifiedData(data.begin() + 24, data.begin() + 32);
	entry.modified = unpackTod(modifiedData);

	entry.attr = static_cast<uint32_t>(data[32] | (data[33] << 8) | (data[34] << 16) | (data[35] << 24));

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

	auto createdData = packTod(entry.created);
	std::copy(createdData.begin(), createdData.end(), data.begin() + 8);

	data[16] = entry.cluster & 0xFF;
	data[17] = (entry.cluster >> 8) & 0xFF;
	data[18] = (entry.cluster >> 16) & 0xFF;
	data[19] = (entry.cluster >> 24) & 0xFF;
	data[20] = entry.dirEntry & 0xFF;
	data[21] = (entry.dirEntry >> 8) & 0xFF;
	data[22] = (entry.dirEntry >> 16) & 0xFF;
	data[23] = (entry.dirEntry >> 24) & 0xFF;

	auto modifiedData = packTod(entry.modified);
	std::copy(modifiedData.begin(), modifiedData.end(), data.begin() + 24);

	data[32] = entry.attr & 0xFF;
	data[33] = (entry.attr >> 8) & 0xFF;
	data[34] = (entry.attr >> 16) & 0xFF;
	data[35] = (entry.attr >> 24) & 0xFF;

	size_t nameLen = std::min(entry.name.size(), static_cast<size_t>(448));
	std::copy(entry.name.begin(), entry.name.begin() + nameLen, data.begin() + 64);

	return data;
}

std::time_t todToTime(const PS2McTod& tod)
{
	// Values on card are in JST (UTC+9).
	// Convert JST -> UTC so the returned time_t is not timezone dependent.
	std::tm timeInfo = {};
	timeInfo.tm_sec = tod.sec;
	timeInfo.tm_min = tod.min;
	timeInfo.tm_hour = tod.hour;
	timeInfo.tm_mday = tod.mday;
	timeInfo.tm_mon = tod.month - 1;
	timeInfo.tm_year = tod.year - 1900;
	timeInfo.tm_isdst = -1;

	std::time_t jstTime;
#if defined(_WIN32)
	jstTime = _mkgmtime(&timeInfo);
#else
	jstTime = timegm(&timeInfo);
#endif

	// JST is UTC+9.
	return jstTime - static_cast<std::time_t>(9 * 60 * 60);
}

PS2McTod timeToTod(std::time_t time)
{
	// Store timestamps on card in JST (UTC+9),
	// regardless of whatever the host system's local timezone is.
	std::time_t jstTime = time + static_cast<std::time_t>(9 * 60 * 60);

	std::tm timeInfo{};
#if defined(_WIN32)
	gmtime_s(&timeInfo, &jstTime);
#else
	gmtime_r(&jstTime, &timeInfo);
#endif
	PS2McTod tod{};
	tod.sec = static_cast<uint8_t>(timeInfo.tm_sec);
	tod.min = static_cast<uint8_t>(timeInfo.tm_min);
	tod.hour = static_cast<uint8_t>(timeInfo.tm_hour);
	tod.mday = static_cast<uint8_t>(timeInfo.tm_mday);
	tod.month = static_cast<uint8_t>(timeInfo.tm_mon + 1);
	tod.year = static_cast<uint16_t>(timeInfo.tm_year + 1900);
	return tod;
}
