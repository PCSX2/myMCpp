// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "PS2McDir.h"
#include "Error.h"
#include <string>
#include <vector>
#include <memory>

class PS2SaveFile;

const char PS2MC_MAGIC[] = "Sony PS2 Memory Card Format ";
const uint32_t PS2MC_FAT_ALLOCATED_BIT = 0x80000000;
const uint32_t PS2MC_FAT_CHAIN_END = 0xFFFFFFFF;
const uint32_t PS2MC_FAT_CHAIN_END_UNALLOC = 0x7FFFFFFF;
const uint32_t PS2MC_FAT_CLUSTER_MASK = 0x7FFFFFFF;
const int PS2MC_MAX_INDIRECT_FAT_CLUSTERS = 32;
const int PS2MC_CLUSTER_SIZE = 1024;
const int PS2MC_INDIRECT_FAT_OFFSET = 0x2000;
const int PS2MC_STANDARD_PAGE_SIZE = 512;
const int PS2MC_STANDARD_PAGES_PER_CARD = 16384;
const int PS2MC_STANDARD_PAGES_PER_ERASE_BLOCK = 16;

class PS2MemoryCard
{
public:
	struct CardFormat
	{
		const char* displayName;
		const char* filterName;
		const char* extension;
		bool usesEcc;
	};

	struct CardInfo
	{
		uint64_t imageSizeBytes = 0;
		uint32_t pageSize = 0;
		uint32_t rawPageSize = 0;
		uint32_t spareSize = 0;
		bool withEcc = false;
		uint8_t cardType = 0;
		uint8_t cardFlags = 0;
		uint32_t pagesPerCluster = 0;
		uint32_t clusterSize = 0;
		uint32_t clustersPerCard = 0;
		uint32_t allocatableOffset = 0;
		uint32_t allocatableCount = 0;
		uint32_t reservedClusters = 0;
		uint32_t backupBlock1 = 0;
		uint32_t backupBlock2 = 0;
		uint32_t badBlockCount = 0;
		uint32_t rootDirCluster = 0;
		uint32_t usedClusters = 0;
		uint32_t freeClusters = 0;
		uint64_t usedBytes = 0;
		uint64_t freeBytes = 0;
	};
	PS2MemoryCard();
	~PS2MemoryCard();

	const Error& GetError() const { return m_error; }

	static const std::vector<CardFormat>& getFormats();
	static bool usesEccForPath(const std::string& path, bool unknownFallback = true);

	bool open(const std::string& filename, bool ignoreEcc = false);
	void close();
	bool create(const std::string& filename, int sizeInMB = 8, bool disableEcc = false);

	std::vector<PS2McDirEntry> listDir(const std::string& path = "/");
	bool hasEntry(const std::string& path);
	PS2McDirEntry getEntry(const std::string& path);
	bool makeDir(const std::string& path);
	bool remove(const std::string& path);

	uint16_t getMode(const std::string& path);
	bool setMode(const std::string& path, uint16_t mode);

	bool setModifiedTime(const std::string& path, std::time_t newTime);
	bool renameEntry(const std::string& path, const std::string& newName);

	std::vector<uint8_t> readFile(const std::string& path, bool quiet = false);
	bool exportFile(const std::string& mcPath, const std::string& hostPath);
	bool writeFile(const std::string& path, const std::vector<uint8_t>& data);

	bool importSaveFile(class PS2SaveFile& save, bool ignoreExisting = false, const std::string& targetDir = "");
	bool exportSaveFile(const std::string& savePath, class PS2SaveFile& save);

	std::vector<uint8_t> getIconData(const std::string& savePath);

	class PS2IconSys* getIconSys(const std::string& savePath);

	std::string getSaveTitle(const std::string& savePath);
	std::string getSaveSubtitle(const std::string& savePath);

	uint32_t getSaveSize(const std::string& savePath);

	uint32_t getFreeSpace();
	uint32_t getAllocatableSpace();

	bool check();
	bool hasEcc() const;

	std::string getPsxTitle(const std::string& savePath);

	bool saveAs(const std::string& filename, bool withEcc = true);

	CardInfo getCardInfo();

private:
	Error m_error;
	class Impl;
	std::unique_ptr<Impl> m_impl;
};
