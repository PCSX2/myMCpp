// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <ctime>

constexpr size_t PS2MC_DIRENT_LENGTH = 512;

// clang-format off
constexpr uint16_t DF_READ       = 0x0001;
constexpr uint16_t DF_WRITE      = 0x0002;
constexpr uint16_t DF_EXECUTE    = 0x0004;
constexpr uint16_t DF_RWX        = DF_READ | DF_WRITE | DF_EXECUTE;
constexpr uint16_t DF_PROTECTED  = 0x0008;
constexpr uint16_t DF_FILE       = 0x0010;
constexpr uint16_t DF_DIR        = 0x0020;
constexpr uint16_t DF_O_DCREAT   = 0x0040;
constexpr uint16_t DF_0080       = 0x0080;
constexpr uint16_t DF_0100       = 0x0100;
constexpr uint16_t DF_O_CREAT    = 0x0200;
constexpr uint16_t DF_0400       = 0x0400;
constexpr uint16_t DF_POCKETSTN  = 0x0800;
constexpr uint16_t DF_PSX        = 0x1000;
constexpr uint16_t DF_HIDDEN     = 0x2000;
constexpr uint16_t DF_4000       = 0x4000;
constexpr uint16_t DF_EXISTS     = 0x8000;
// clang-format on

struct PS2McTod
{
	uint8_t sec = 0;
	uint8_t min = 0;
	uint8_t hour = 0;
	uint8_t mday = 0;
	uint8_t month = 0;
	uint16_t year = 0;
};

struct PS2McDirEntry
{
	uint16_t mode = 0;
	uint16_t unused = 0;
	uint32_t length = 0;
	PS2McTod created{};
	uint32_t cluster = 0;
	uint32_t dirEntry = 0;
	PS2McTod modified{};
	uint32_t attr = 0;
	std::string name;
};

std::string zeroTerminate(const std::string& s);
PS2McTod unpackTod(const std::vector<uint8_t>& data);
std::vector<uint8_t> packTod(const PS2McTod& tod);
PS2McDirEntry unpackDirEntry(const std::vector<uint8_t>& data);
std::vector<uint8_t> packDirEntry(const PS2McDirEntry& entry);
std::time_t todToTime(const PS2McTod& tod);
PS2McTod timeToTod(std::time_t time);
