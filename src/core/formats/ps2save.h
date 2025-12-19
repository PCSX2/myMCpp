// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "ps2mc_dir.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>

const char PS2SAVE_MAX_MAGIC[] = "Ps2PowerSave";
const char PS2SAVE_SPS_MAGIC[] = "\x0d\0\0\0SharkPortSave";
const char PS2SAVE_CBS_MAGIC[] = "CFU\0";
const char PS2SAVE_NPO_MAGIC[] = "nPort";
const char PS2SAVE_PSV_MAGIC[] = "\x00VSP";

class PS2SaveError : public std::runtime_error
{
public:
	explicit PS2SaveError(const std::string& msg)
		: std::runtime_error(msg)
	{
	}
};

class PS2SaveCorrupt : public PS2SaveError
{
public:
	explicit PS2SaveCorrupt(const std::string& msg)
		: PS2SaveError("Corrupt save file: " + msg)
	{
	}
};

enum class SaveFormat
{
	UNKNOWN,
	MAX_DRIVE, // .max, .psu
	SHARKPORT, // .sps
	CODEBREAKER, // .cbs
	XPORT, // .xps
	EMS, // .psu
	PSV, // .psv
	NPORT // .npo
};

struct PS2SaveEntry
{
	PS2McDirEntry dirEntry;
	std::vector<uint8_t> data;
};

class PS2SaveFile
{
public:
	PS2SaveFile();
	~PS2SaveFile();

	void load(const std::string& filename);
	void save(const std::string& filename, SaveFormat format = SaveFormat::MAX_DRIVE);

	std::vector<PS2SaveEntry>& getEntries();
	const std::vector<PS2SaveEntry>& getEntries() const;

	std::string getTitle() const;
	void setTitle(const std::string& title);
	SaveFormat getFormat() const;

	static SaveFormat detectFormat(const std::string& filename);
	static std::string formatToExtension(SaveFormat format);

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

std::string makeLongname(const std::string& dirname, const PS2SaveFile& save);
std::string sjisToUtf8(const std::string& sjis);
std::string utf8ToSjis(const std::string& utf8);
std::wstring sjisToWide(const std::string& sjis);
std::string wideToSjis(const std::wstring& wide);
