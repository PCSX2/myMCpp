// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <ctime>

const int PS2MC_DIRENT_LENGTH = 512;

// clang-format off
const uint16_t DF_READ       = 0x0001;
const uint16_t DF_WRITE      = 0x0002;
const uint16_t DF_EXECUTE    = 0x0004;
const uint16_t DF_RWX        = DF_READ | DF_WRITE | DF_EXECUTE;
const uint16_t DF_PROTECTED  = 0x0008;
const uint16_t DF_FILE       = 0x0010;
const uint16_t DF_DIR        = 0x0020;
const uint16_t DF_O_DCREAT   = 0x0040;
const uint16_t DF_0080       = 0x0080;
const uint16_t DF_0100       = 0x0100;
const uint16_t DF_O_CREAT    = 0x0200;
const uint16_t DF_0400       = 0x0400;
const uint16_t DF_POCKETSTN  = 0x0800;
const uint16_t DF_PSX        = 0x1000;
const uint16_t DF_HIDDEN     = 0x2000;
const uint16_t DF_4000       = 0x4000;
const uint16_t DF_EXISTS     = 0x8000;
// clang-format on

struct PS2McTod
{
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t mday;
	uint8_t month;
	uint16_t year;
};

struct PS2McDirEntry
{
	uint16_t mode;
	uint16_t unused;
	uint32_t length;
	PS2McTod created;
	uint32_t cluster;
	uint32_t dirEntry;
	PS2McTod modified;
	uint32_t attr;
	std::string name;
};

std::string zeroTerminate(const std::string& s);
PS2McTod unpackTod(const std::vector<uint8_t>& data);
std::vector<uint8_t> packTod(const PS2McTod& tod);
PS2McDirEntry unpackDirEntry(const std::vector<uint8_t>& data);
std::vector<uint8_t> packDirEntry(const PS2McDirEntry& entry);
std::time_t todToTime(const PS2McTod& tod);
PS2McTod timeToTod(std::time_t time);
