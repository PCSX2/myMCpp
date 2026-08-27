// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+
// PS2 memory card layout and algorithms adapted from mymc++ / mymc (Florian Märkl, Ross Ridge) and public PS2 save format documentation.

#include "PS2MemoryCard.h"
#include "PS2McEcc.h"
#include "PS2McDir.h"
#include "PS2IconSys.h"
#include "PS2SaveFile.h"
#include "common/Logger.h"
#include "common/sjis.h"
#include "common/round.h"
#include <fstream>
#include <cstring>
#include <vector>
#include <array>
#include <map>
#include <memory>
#include <algorithm>
#include <ctime>
#include <filesystem>

template <typename T>
void writeLE(std::vector<uint8_t>& buf, size_t offset, T value)
{
	for (size_t i = 0; i < sizeof(T); ++i)
	{
		buf[offset + i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
	}
}

template <typename T>
T readLE(const std::vector<uint8_t>& buf, size_t offset)
{
	T value = 0;
	for (size_t i = 0; i < sizeof(T); ++i)
	{
		value |= (static_cast<T>(buf[offset + i]) << (i * 8));
	}
	return value;
}

const std::vector<PS2MemoryCard::CardFormat>& PS2MemoryCard::getFormats()
{
	static const std::vector<CardFormat> formats = {
		{"PCSX2", "PCSX2 (*.ps2)", "ps2", true},
		{"MemCard PRO2", "MemCard PRO2 (*.mc2)", "mc2", false},
		{"SD2PSX", "SD2PSX (*.mcd)", "mcd", false},
		{"PS3 VMC", "PS3 VMC (*.vm2)", "vm2", true},
		{"Raw VMC", "Raw VMC (*.bin)", "bin", false},
		{"Raw VMC", "Raw VMC (*.vmc)", "vmc", false},
		{"Raw VMC", "Raw VMC (*.mc)", "mc", false},
	};
	return formats;
}

bool PS2MemoryCard::usesEccForPath(const std::string& path, bool unknownFallback)
{
	std::filesystem::path p(path);
	std::string ext = p.extension().string();
	if (ext.empty())
	{
		return unknownFallback;
	}

	ext = ext.substr(1);

	for (char& c : ext)
	{
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}

	for (const CardFormat& format : getFormats())
	{
		if (ext == format.extension)
		{
			return format.usesEcc;
		}
	}
	return unknownFallback;
}

static bool renameReplace(const std::filesystem::path& target, const std::filesystem::path& tmp, Error& error)
{
	std::error_code ec;
	std::filesystem::rename(tmp, target, ec);
	if (ec)
	{
		ec.clear();
		std::filesystem::remove(target, ec);
		ec.clear();
		std::filesystem::rename(tmp, target, ec);
	}
	if (ec)
	{
		return error.Fail("Failed to replace memory card file: " + ec.message());
	}
	return true;
}

class PS2MemoryCard::Impl
{
public:
	Error& m_error;
	std::fstream m_file;
	std::string m_filename;

	uint32_t m_pageSize = 512;
	uint32_t m_pagesPerCluster = 2;
	uint32_t m_clusterSize = 1024;
	uint32_t m_pagesPerEraseBlock = 16;
	uint32_t m_spareSize = 16; // ECC spare data size
	uint32_t m_rawPageSize = 528; // m_pageSize + m_spareSize

	bool m_withEcc = true;
	bool m_ignoreEcc = false;

	uint32_t m_clustersPerCard = 8192;
	uint32_t m_allocatableClusterOffset = 0;
	uint32_t m_allocatableClusterEnd = 0;
	uint32_t m_rootDirFatCluster = 0;

	uint32_t m_entriesPerCluster = 256; // m_clusterSize / 4

	std::vector<uint32_t> m_fat;

	std::array<uint32_t, 32> m_indirectFatClusterList = {};

	uint8_t m_cardType = 0;
	uint8_t m_cardFlags = 0;
	uint32_t m_backupBlock1 = 0;
	uint32_t m_backupBlock2 = 0;
	std::array<uint32_t, 32> m_badBlockList = {};

	bool m_modified = false;
	std::map<uint32_t, std::vector<uint8_t>> m_pageCache;
	std::map<uint32_t, std::vector<uint8_t>> m_fatClusterCache;

	explicit Impl(Error& error)
		: m_error(error)
	{
	}

	void calculateDerived();
	bool parseSuperblockFields(const std::vector<uint8_t>& page);
	std::vector<uint8_t> loadSuperblock();
	bool applyLayout(bool ecc, std::vector<uint8_t> sb_page);
	bool rootDirectoryOk();
	bool readSuperblock();
	std::vector<uint8_t> readPage(uint32_t page_num);
	bool writePage(uint32_t page_num, const std::vector<uint8_t>& data);
	std::vector<uint8_t> readCluster(uint32_t cluster_num);
	bool writeCluster(uint32_t cluster_num, const std::vector<uint8_t>& data);
	std::vector<uint32_t> readFatCluster(uint32_t fat_cluster_num);
	bool readFatFromCard();
	bool writeFatToCard();
	uint32_t lookupFat(uint32_t cluster_num);
	bool findEntry(const std::string& path, uint32_t& parent_cluster, PS2McDirEntry& out_entry, bool* path_not_found = nullptr, bool quiet = false);
	std::vector<PS2McDirEntry> readDirents(uint32_t dir_cluster);
	bool writeDirents(uint32_t dir_cluster, const std::vector<PS2McDirEntry>& entries);
	bool syncParentDirectoryEntryLength(uint32_t child_dir_cluster);
	uint32_t allocateCluster();
	std::vector<uint32_t> allocateClusters(uint32_t count);
	void freeClusterChain(uint32_t start_cluster);

	void initCardParameters(int sizeInMB, bool disableEcc);
	void calculateFatLayout(uint32_t& first_ifc, uint32_t& indirect_fat_clusters, uint32_t& fat_clusters);
	bool createEmptyCardFile(const std::string& filename, uint64_t totalBytes);
	bool writeSuperblock(uint32_t first_ifc, uint32_t indirect_fat_clusters, uint32_t good_block1, uint32_t good_block2);
	bool initIndirectFatClusters(uint32_t first_ifc, uint32_t indirect_fat_clusters, uint32_t epc);
	bool initRootDirectory();
};

void PS2MemoryCard::Impl::calculateDerived()
{
	m_spareSize = divRoundUp(m_pageSize, 128) * 4;
	m_rawPageSize = m_pageSize + m_spareSize;
	m_clusterSize = m_pageSize * m_pagesPerCluster;
	m_entriesPerCluster = m_clusterSize / 4; // 4 bytes per FAT entry
}

bool PS2MemoryCard::Impl::parseSuperblockFields(const std::vector<uint8_t>& page)
{
	m_pageSize = readLE<uint16_t>(page, 40);
	m_pagesPerCluster = readLE<uint16_t>(page, 42);
	m_pagesPerEraseBlock = readLE<uint16_t>(page, 44);

	if (m_pageSize == 0 || m_pagesPerCluster == 0)
	{
		return m_error.Fail("Invalid page/cluster size in superblock");
	}

	calculateDerived();

	m_clustersPerCard = readLE<uint32_t>(page, 48);
	m_allocatableClusterOffset = readLE<uint32_t>(page, 52);
	m_allocatableClusterEnd = readLE<uint32_t>(page, 56);
	m_rootDirFatCluster = readLE<uint32_t>(page, 60);

	for (int i = 0; i < 32; ++i)
	{
		m_indirectFatClusterList[i] = readLE<uint32_t>(page, 80 + i * 4);
	}

	if (page.size() > 0x44 + 4)
	{
		m_backupBlock1 = readLE<uint32_t>(page, 0x40);
		m_backupBlock2 = readLE<uint32_t>(page, 0x44);
	}

	if (page.size() > 0x151)
	{
		m_cardType = page[0x150];
		m_cardFlags = page[0x151];
	}

	if (page.size() > 0xD0 + 32 * 4)
	{
		for (int i = 0; i < 32; ++i)
		{
			m_badBlockList[i] = readLE<uint32_t>(page, 0xD0 + i * 4);
		}
	}

	if (m_allocatableClusterEnd == 0 || m_allocatableClusterEnd > 1000000)
	{
		m_allocatableClusterEnd = m_clustersPerCard;
	}
	return true;
}

std::vector<uint8_t> PS2MemoryCard::Impl::readPage(uint32_t page_num)
{
	if (!m_file.is_open())
	{
		m_error.Fail("Memory card not open");
		return {};
	}

	if (m_pageCache.count(page_num))
	{
		return m_pageCache[page_num];
	}

	uint64_t offset = static_cast<uint64_t>(page_num) * m_rawPageSize;

	m_file.seekg(static_cast<std::streamoff>(offset));
	if (!m_file)
	{
		m_error.Fail("Seek failed");
		return {};
	}

	std::vector<uint8_t> page_data(m_pageSize);
	m_file.read(reinterpret_cast<char*>(page_data.data()), m_pageSize);

	if (m_file.gcount() != static_cast<std::streamsize>(m_pageSize))
	{
		m_error.Fail("Failed to read complete page");
		return {};
	}

	if (m_spareSize > 0)
	{
		std::vector<uint8_t> spare(m_spareSize);
		m_file.read(reinterpret_cast<char*>(spare.data()), m_spareSize);

		if (m_file.gcount() != static_cast<std::streamsize>(m_spareSize))
		{
			m_error.Fail("Failed to read ECC spare data");
			return {};
		}

		const int ecc_status = eccCheckPage(page_data, spare, m_pageSize);
		if (ecc_status == ECC_CHECK_FAILED && !m_ignoreEcc)
		{
			m_error.Fail("Unrecoverable ECC error (page " + std::to_string(page_num) + ")");
			return {};
		}
	}

	m_pageCache[page_num] = page_data;
	return page_data;
}

bool PS2MemoryCard::Impl::writePage(uint32_t page_num, const std::vector<uint8_t>& data)
{
	if (!m_file.is_open())
	{
		return m_error.Fail("Memory card not open");
	}

	if (data.size() != m_pageSize)
	{
		return m_error.Fail("Invalid page data size");
	}

	uint64_t offset = static_cast<uint64_t>(page_num) * m_rawPageSize;
	m_file.seekp(static_cast<std::streamoff>(offset));
	if (!m_file)
	{
		return m_error.Fail("Seek failed");
	}

	m_file.write(reinterpret_cast<const char*>(data.data()), m_pageSize);
	if (!m_file)
	{
		return m_error.Fail("Failed to write page");
	}

	if (m_spareSize > 0)
	{
		std::vector<uint8_t> spare(m_spareSize, 0);
		auto ecc_data = eccCalculatePage(data, m_pageSize);
		for (size_t i = 0; i < ecc_data.size() && i < m_spareSize; ++i)
		{
			spare[i] = ecc_data[i];
		}
		m_file.write(reinterpret_cast<const char*>(spare.data()), m_spareSize);
		if (!m_file)
		{
			return m_error.Fail("Failed to write page");
		}
	}

	m_modified = true;
	m_pageCache[page_num] = data;
	return true;
}

std::vector<uint8_t> PS2MemoryCard::Impl::readCluster(uint32_t cluster_num)
{
	std::vector<uint8_t> result;
	result.reserve(m_clusterSize);

	for (uint32_t i = 0; i < m_pagesPerCluster; ++i)
	{
		uint32_t page_num = cluster_num * m_pagesPerCluster + i;
		auto page_data = readPage(page_num);
		if (page_data.empty())
		{
			return {};
		}
		result.insert(result.end(), page_data.begin(), page_data.end());
	}

	return result;
}

bool PS2MemoryCard::Impl::writeCluster(uint32_t cluster_num, const std::vector<uint8_t>& data)
{
	if (data.size() != m_clusterSize)
	{
		return m_error.Fail("Invalid cluster data size");
	}

	for (uint32_t i = 0; i < m_pagesPerCluster; ++i)
	{
		uint32_t page_num = cluster_num * m_pagesPerCluster + i;
		std::vector<uint8_t> page_data(
			data.begin() + static_cast<size_t>(i) * m_pageSize,
			data.begin() + static_cast<size_t>(i + 1) * m_pageSize);
		if (!writePage(page_num, page_data))
		{
			return false;
		}
	}
	return true;
}

std::vector<uint32_t> PS2MemoryCard::Impl::readFatCluster(uint32_t fat_cluster_num)
{
	if (m_fatClusterCache.count(fat_cluster_num))
	{
		auto cluster_data = m_fatClusterCache[fat_cluster_num];
		std::vector<uint32_t> result(m_entriesPerCluster);
		for (uint32_t i = 0; i < m_entriesPerCluster; ++i)
		{
			result[i] = readLE<uint32_t>(cluster_data, i * 4);
		}
		return result;
	}

	auto cluster_data = readCluster(fat_cluster_num);
	if (cluster_data.empty())
	{
		return {};
	}
	std::vector<uint32_t> result(m_entriesPerCluster);
	for (uint32_t i = 0; i < m_entriesPerCluster; ++i)
	{
		result[i] = readLE<uint32_t>(cluster_data, i * 4);
	}

	m_fatClusterCache[fat_cluster_num] = cluster_data;
	return result;
}

bool PS2MemoryCard::Impl::readFatFromCard()
{
	m_fat.clear();
	m_fat.resize(m_allocatableClusterEnd, PS2MC_FAT_CHAIN_END_UNALLOC);

	uint32_t fat_entry = 0;

	for (uint32_t dbl_offset = 0; dbl_offset < 32 && fat_entry < m_fat.size(); ++dbl_offset)
	{
		uint32_t indirect_cluster = m_indirectFatClusterList[dbl_offset];

		if (indirect_cluster == 0 || indirect_cluster == 0xFFFFFFFF)
		{
			continue;
		}

		auto indirect_data = readCluster(indirect_cluster);
		if (indirect_data.empty())
		{
			return false;
		}

		for (uint32_t indirect_offset = 0; indirect_offset < m_entriesPerCluster && fat_entry < m_fat.size(); ++indirect_offset)
		{
			uint32_t fat_cluster_ptr = readLE<uint32_t>(indirect_data, indirect_offset * 4);

			if (fat_cluster_ptr == 0 || fat_cluster_ptr == 0xFFFFFFFF)
			{
				continue;
			}

			auto fat_cluster = readFatCluster(fat_cluster_ptr);
			if (fat_cluster.empty())
			{
				return false;
			}

			for (uint32_t j = 0; j < fat_cluster.size(); ++j)
			{
				if (fat_entry >= m_fat.size())
				{
					break;
				}
				m_fat[fat_entry] = fat_cluster[j];
				fat_entry++;
			}
		}
	}
	return true;
}

std::vector<uint8_t> PS2MemoryCard::Impl::loadSuperblock()
{
	m_file.seekg(0, std::ios::beg);
	std::vector<uint8_t> sb_page(m_pageSize);
	m_file.read(reinterpret_cast<char*>(sb_page.data()), m_pageSize);
	if (m_file.gcount() != static_cast<std::streamsize>(m_pageSize))
	{
		m_error.Fail("Failed to read superblock");
		return {};
	}

	const char* MAGIC = "Sony PS2 Memory Card Format ";
	if (sb_page.size() < 28 || std::memcmp(sb_page.data(), MAGIC, 28) != 0)
	{
		m_error.Fail("Invalid memory card magic");
		return {};
	}

	if (!parseSuperblockFields(sb_page))
	{
		return {};
	}
	return sb_page;
}

bool PS2MemoryCard::Impl::applyLayout(bool ecc, std::vector<uint8_t> sb_page)
{
	m_pageCache.clear();
	m_fatClusterCache.clear();
	if (!parseSuperblockFields(sb_page))
	{
		return false;
	}

	if (ecc)
	{
		m_withEcc = true;
		if (m_spareSize > 0)
		{
			std::vector<uint8_t> spare(m_spareSize);
			m_file.seekg(m_pageSize, std::ios::beg);
			m_file.read(reinterpret_cast<char*>(spare.data()), m_spareSize);
			if (m_file.gcount() == static_cast<std::streamsize>(m_spareSize))
			{
				std::vector<uint8_t> page = sb_page;
				const int ecc_status = eccCheckPage(page, spare, static_cast<int>(m_pageSize));
				if (ecc_status != ECC_CHECK_FAILED)
				{
					sb_page = std::move(page);
					if (ecc_status == ECC_CHECK_CORRECTED)
					{
						if (!parseSuperblockFields(sb_page))
						{
							return false;
						}
					}
				}
			}
		}
	}
	else
	{
		m_spareSize = 0;
		m_rawPageSize = m_pageSize;
		m_withEcc = false;
	}

	m_pageCache[0] = std::move(sb_page);
	return readFatFromCard();
}

bool PS2MemoryCard::Impl::rootDirectoryOk()
{
	const auto entries = readDirents(m_rootDirFatCluster);
	return entries.size() >= 2 &&
	       entries[0].name == "." &&
	       entries[1].name == ".." &&
	       (entries[0].mode & DF_DIR) &&
	       (entries[1].mode & DF_DIR);
}

bool PS2MemoryCard::Impl::readSuperblock()
{
	auto sb_page = loadSuperblock();
	if (sb_page.empty())
	{
		return false;
	}

	const uint32_t total_pages = m_clustersPerCard * m_pagesPerCluster;
	const uint32_t ecc_spare = divRoundUp(m_pageSize, 128) * 4;
	const uint64_t expected_with_ecc = static_cast<uint64_t>(total_pages) * (m_pageSize + ecc_spare);

	m_file.seekg(0, std::ios::end);
	const uint64_t file_size = static_cast<uint64_t>(m_file.tellg());

	bool ecc = m_withEcc;
	if (file_size < expected_with_ecc)
	{
		ecc = false;
	}

	return applyLayout(ecc, std::move(sb_page));
}

uint32_t PS2MemoryCard::Impl::lookupFat(uint32_t cluster_num)
{
	if (cluster_num >= m_fat.size())
	{
		m_error.Fail("FAT index out of range");
		return PS2MC_FAT_CHAIN_END;
	}
	return m_fat[cluster_num];
}

bool PS2MemoryCard::Impl::findEntry(const std::string& path, uint32_t& parent_cluster, PS2McDirEntry& out_entry, bool* path_not_found, bool quiet)
{
	if (path_not_found)
	{
		*path_not_found = false;
	}

	std::vector<std::string> parts;
	if (path != "/")
	{
		size_t pos = 0;
		while (pos < path.size())
		{
			size_t next = path.find('/', pos);
			if (next == std::string::npos)
			{
				next = path.size();
			}
			if (next > pos)
			{
				parts.push_back(path.substr(pos, next - pos));
			}
			pos = next + 1;
		}
	}

	if (parts.empty())
	{
		out_entry = {};
		out_entry.mode = DF_DIR | DF_EXISTS;
		out_entry.name = "/";
		out_entry.cluster = m_rootDirFatCluster;
		return true;
	}

	uint32_t current_cluster = m_rootDirFatCluster;
	parent_cluster = current_cluster;
	PS2McDirEntry found_entry{};

	for (size_t i = 0; i < parts.size(); ++i)
	{
		const auto& part = parts[i];
		auto entries = readDirents(current_cluster);
		if (m_error.IsValid())
		{
			return false;
		}

		bool found = false;
		for (const auto& e : entries)
		{
			if (e.name == part)
			{
				found_entry = e;
				found = true;
				break;
			}
		}

		if (!found)
		{
			if (path_not_found)
			{
				*path_not_found = true;
			}
			if (!quiet)
			{
				m_error.Fail("Path not found: " + path);
			}
			return false;
		}

		if (i < parts.size() - 1)
		{
			if (!(found_entry.mode & DF_DIR))
			{
				if (!quiet)
				{
					m_error.Fail("Path component is not a directory");
				}
				return false;
			}
			parent_cluster = current_cluster;
			current_cluster = found_entry.cluster;
		}
	}

	out_entry = found_entry;
	return true;
}

std::vector<PS2McDirEntry> PS2MemoryCard::Impl::readDirents(uint32_t dir_cluster)
{
	std::vector<PS2McDirEntry> entries;
	if (dir_cluster >= m_fat.size())
	{
		m_error.Fail("Directory cluster out of range");
		return {};
	}

	uint32_t current_cluster = dir_cluster;
	int iteration = 0;

	while (current_cluster != PS2MC_FAT_CHAIN_END && current_cluster < m_fat.size())
	{
		iteration++;
		if (iteration > 1000)
		{
			m_error.Fail("Directory chain exceeds safety limit");
			return {};
		}
		uint32_t disk_cluster = current_cluster + m_allocatableClusterOffset;
		auto cluster_data = readCluster(disk_cluster);
		if (cluster_data.empty())
		{
			return {};
		}

		uint32_t entries_per_cluster_block = m_clusterSize / PS2MC_DIRENT_LENGTH;

		for (uint32_t i = 0; i < entries_per_cluster_block; ++i)
		{
			uint32_t offset = i * PS2MC_DIRENT_LENGTH;
			if (offset + PS2MC_DIRENT_LENGTH > cluster_data.size())
			{
				break;
			}

			std::vector<uint8_t> ent_data(
				cluster_data.begin() + offset,
				cluster_data.begin() + offset + PS2MC_DIRENT_LENGTH);

			auto entry = unpackDirEntry(ent_data);

			if (!(entry.mode & DF_EXISTS))
			{
				continue;
			}

			entries.push_back(entry);
		}

		if (current_cluster >= m_fat.size())
		{
			break;
		}

		uint32_t next = m_fat[current_cluster];

		if ((next & PS2MC_FAT_CLUSTER_MASK) == PS2MC_FAT_CHAIN_END_UNALLOC)
		{
			break;
		}

		next = next & PS2MC_FAT_CLUSTER_MASK;

		if (next >= m_fat.size() || next == PS2MC_FAT_CHAIN_END_UNALLOC)
		{
			break;
		}

		current_cluster = next;
	}

	return entries;
}

bool PS2MemoryCard::Impl::writeDirents(uint32_t dir_cluster, const std::vector<PS2McDirEntry>& entries)
{
	if (dir_cluster >= m_fat.size())
	{
		return m_error.Fail("Directory cluster out of range");
	}
	if (entries.empty())
	{
		return true;
	}

	uint32_t current_cluster = dir_cluster;
	uint32_t entry_idx = 0;
	int iteration = 0;

	while (entry_idx < entries.size())
	{
		iteration++;
		if (iteration > 1000)
		{
			return m_error.Fail("Directory chain exceeds safety limit");
		}

		std::vector<uint8_t> cluster_data(m_clusterSize, 0);
		uint32_t entries_per_cluster_block = m_clusterSize / PS2MC_DIRENT_LENGTH;

		for (uint32_t i = 0; i < entries_per_cluster_block && entry_idx < entries.size(); ++i, ++entry_idx)
		{
			auto packed = packDirEntry(entries[entry_idx]);
			uint32_t offset = i * PS2MC_DIRENT_LENGTH;
			std::copy(packed.begin(), packed.end(), cluster_data.begin() + offset);
		}

		uint32_t disk_cluster = current_cluster + m_allocatableClusterOffset;
		if (!writeCluster(disk_cluster, cluster_data))
		{
			return false;
		}

		if (entry_idx < entries.size())
		{
			uint32_t next = m_fat[current_cluster] & PS2MC_FAT_CLUSTER_MASK;

			if (next >= m_fat.size() || next == PS2MC_FAT_CHAIN_END_UNALLOC || next == PS2MC_FAT_CHAIN_END)
			{
				uint32_t new_cluster = allocateCluster();
				if (new_cluster == PS2MC_FAT_CHAIN_END)
				{
					return false;
				}
				m_fat[current_cluster] = (m_fat[current_cluster] & ~PS2MC_FAT_CLUSTER_MASK) | (new_cluster | PS2MC_FAT_ALLOCATED_BIT);
				m_fat[new_cluster] = PS2MC_FAT_CHAIN_END;
				current_cluster = new_cluster;
			}
			else
			{
				current_cluster = next;
			}
		}
		else
		{
			break;
		}
	}

	// Terminate the FAT chain at the current cluster if there was an old chain pointing further
	uint32_t next = m_fat[current_cluster] & PS2MC_FAT_CLUSTER_MASK;
	if (next != PS2MC_FAT_CHAIN_END && next != PS2MC_FAT_CHAIN_END_UNALLOC && next < m_fat.size())
	{
		m_fat[current_cluster] = PS2MC_FAT_CHAIN_END;

		// Free all subsequent clusters in the old chain
		while (next != PS2MC_FAT_CHAIN_END && next != PS2MC_FAT_CHAIN_END_UNALLOC && next < m_fat.size())
		{
			uint32_t temp = m_fat[next] & PS2MC_FAT_CLUSTER_MASK;
			m_fat[next] = PS2MC_FAT_CHAIN_END_UNALLOC;
			next = temp;
		}
	}
	return true;
}

bool PS2MemoryCard::Impl::syncParentDirectoryEntryLength(uint32_t child_dir_cluster)
{
	auto child = readDirents(child_dir_cluster);
	if (m_error.IsValid())
	{
		return false;
	}
	if (child.empty() || child[0].name != ".")
	{
		return true;
	}
	const uint32_t newLen = child[0].length;
	const uint32_t ancestor = child[0].cluster;
	const uint32_t slot = child[0].dirEntry;

	auto ancestor_entries = readDirents(ancestor);
	if (m_error.IsValid())
	{
		return false;
	}
	bool updated = false;
	if (slot < ancestor_entries.size() && (ancestor_entries[slot].mode & DF_DIR) && ancestor_entries[slot].cluster == child_dir_cluster)
	{
		ancestor_entries[slot].length = newLen;
		updated = true;
	}
	else
	{
		for (auto& e : ancestor_entries)
		{
			if ((e.mode & DF_DIR) && (e.mode & DF_EXISTS) && e.cluster == child_dir_cluster)
			{
				e.length = newLen;
				updated = true;
				break;
			}
		}
	}
	if (updated)
	{
		return writeDirents(ancestor, ancestor_entries);
	}
	return true;
}

uint32_t PS2MemoryCard::Impl::allocateCluster()
{
	for (uint32_t i = 0; i < m_fat.size(); ++i)
	{
		if ((m_fat[i] & PS2MC_FAT_ALLOCATED_BIT) == 0 && m_fat[i] == PS2MC_FAT_CHAIN_END_UNALLOC)
		{
			m_fat[i] = PS2MC_FAT_CHAIN_END; // Mark as end of chain
			m_modified = true;
			return i;
		}
	}

	m_error.Fail("No free clusters available on memory card");
	return PS2MC_FAT_CHAIN_END;
}

std::vector<uint32_t> PS2MemoryCard::Impl::allocateClusters(uint32_t count)
{
	std::vector<uint32_t> clusters;
	clusters.reserve(count);

	for (uint32_t i = 0; i < count; ++i)
	{
		uint32_t cluster = allocateCluster();
		if (cluster == PS2MC_FAT_CHAIN_END)
		{
			for (uint32_t c : clusters)
			{
				m_fat[c] = PS2MC_FAT_CHAIN_END_UNALLOC;
			}
			return {};
		}
		clusters.push_back(cluster);

		if (i > 0)
		{
			m_fat[clusters[i - 1]] = cluster | PS2MC_FAT_ALLOCATED_BIT;
		}
	}

	if (!clusters.empty())
	{
		m_fat[clusters.back()] = PS2MC_FAT_CHAIN_END;
	}

	return clusters;
}

void PS2MemoryCard::Impl::freeClusterChain(uint32_t start_cluster)
{
	uint32_t current = start_cluster;

	while (current < m_fat.size())
	{
		const uint32_t raw = m_fat[current];
		const uint32_t masked = raw & PS2MC_FAT_CLUSTER_MASK;

		m_fat[current] = PS2MC_FAT_CHAIN_END_UNALLOC;

		if (raw == PS2MC_FAT_CHAIN_END || masked >= m_fat.size() || masked == PS2MC_FAT_CHAIN_END_UNALLOC)
		{
			break;
		}

		current = masked;
	}

	m_modified = true;
}

bool PS2MemoryCard::Impl::writeFatToCard()
{
	uint32_t fat_entry_idx = 0;

	for (int i = 0; i < 32; ++i)
	{
		uint32_t indirect_cluster = m_indirectFatClusterList[i];
		if (indirect_cluster == 0 || indirect_cluster == 0xFFFFFFFF)
		{
			continue;
		}

		auto indirect_data = readCluster(indirect_cluster);
		if (indirect_data.empty())
		{
			return false;
		}
		for (uint32_t j = 0; j < m_entriesPerCluster; ++j)
		{
			uint32_t fat_cluster_phys = readLE<uint32_t>(indirect_data, j * 4);

			if (fat_cluster_phys == 0 || fat_cluster_phys == 0xFFFFFFFF)
			{
				continue;
			}

			if (fat_entry_idx >= m_fat.size())
			{
				break;
			}

			std::vector<uint8_t> fat_data(m_clusterSize, 0);

			for (uint32_t k = 0; k < m_entriesPerCluster && fat_entry_idx < m_fat.size(); ++k)
			{
				uint32_t entry = m_fat[fat_entry_idx++];
				writeLE(fat_data, k * 4, entry);
			}

			if (!writeCluster(fat_cluster_phys, fat_data))
			{
				return false;
			}

			if (fat_entry_idx >= m_fat.size())
			{
				break;
			}
		}

		if (fat_entry_idx >= m_fat.size())
		{
			break;
		}
	}
	return true;
}

PS2MemoryCard::PS2MemoryCard()
	: m_impl(std::make_unique<Impl>(m_error))
{
}

PS2MemoryCard::~PS2MemoryCard()
{
	close();
}

bool PS2MemoryCard::open(const std::string& path, bool ignoreEcc)
{
	m_error.Clear();
	m_impl->m_filename = path;
	m_impl->m_ignoreEcc = ignoreEcc;
	m_impl->m_file.open(path, std::ios::in | std::ios::out | std::ios::binary);

	if (!m_impl->m_file)
	{
		m_impl->m_file.open(path, std::ios::in | std::ios::binary);
		if (!m_impl->m_file)
		{
			return m_error.Fail("Failed to open memory card: " + path);
		}
	}

	auto sb_page = m_impl->loadSuperblock();
	if (sb_page.empty())
	{
		close();
		return false;
	}

	const uint32_t total_pages = m_impl->m_clustersPerCard * m_impl->m_pagesPerCluster;
	const uint32_t ecc_spare = divRoundUp(m_impl->m_pageSize, 128) * 4;
	const uint64_t expected_with_ecc =
		static_cast<uint64_t>(total_pages) * (m_impl->m_pageSize + ecc_spare);

	m_impl->m_file.seekg(0, std::ios::end);
	const uint64_t file_size = static_cast<uint64_t>(m_impl->m_file.tellg());
	const bool can_ecc = file_size >= expected_with_ecc;

	auto try_layout = [this, &sb_page](bool ecc) {
		return m_impl->applyLayout(ecc, sb_page) && m_impl->rootDirectoryOk();
	};

	m_error.Clear();
	if (can_ecc && try_layout(true))
	{
		return true;
	}
	m_error.Clear();
	if (try_layout(false))
	{
		return true;
	}

	if (m_error.IsValid())
	{
		close();
		return false;
	}
	close();
	return m_error.Fail("Root directory damaged");
}

void PS2MemoryCard::close()
{
	if (m_impl->m_file.is_open())
	{
		m_impl->m_file.flush();
		m_impl->m_file.close();
	}
	m_impl->m_pageCache.clear();
	m_impl->m_fatClusterCache.clear();
}

void PS2MemoryCard::Impl::initCardParameters(int sizeInMB, bool disableEcc)
{
	m_pageSize = 512;
	m_pagesPerCluster = 2;
	m_pagesPerEraseBlock = 16;
	m_clustersPerCard = (sizeInMB * 1024 * 1024) / 1024;
	m_withEcc = !disableEcc;

	if (disableEcc)
	{
		m_spareSize = 0;
		m_rawPageSize = m_pageSize;
	}
	else
	{
		calculateDerived();
	}
}

void PS2MemoryCard::Impl::calculateFatLayout(uint32_t& first_ifc, uint32_t& indirect_fat_clusters_out, uint32_t& fat_clusters_out)
{
	first_ifc = 8;
	uint32_t epc = m_clusterSize / 4;

	uint32_t allocatable_clusters_est = m_clustersPerCard - (first_ifc + 2);
	uint32_t fat_clusters = (allocatable_clusters_est + epc - 1) / epc;
	uint32_t indirect_fat_clusters = (fat_clusters + epc - 1) / epc;
	if (indirect_fat_clusters > 32)
	{
		indirect_fat_clusters = 32;
		fat_clusters = indirect_fat_clusters * epc;
	}

	m_allocatableClusterOffset = first_ifc + indirect_fat_clusters + fat_clusters;

	uint32_t pages_per_card = m_clustersPerCard * m_pagesPerCluster;
	uint32_t erase_blocks_per_card = pages_per_card / m_pagesPerEraseBlock;
	uint32_t good_block2 = erase_blocks_per_card - 2;
	uint32_t clusters_per_erase_block = m_pagesPerEraseBlock / m_pagesPerCluster;

	m_allocatableClusterEnd = (good_block2 * clusters_per_erase_block) - m_allocatableClusterOffset;
	m_rootDirFatCluster = 0;

	indirect_fat_clusters_out = indirect_fat_clusters;
	fat_clusters_out = fat_clusters;
}

bool PS2MemoryCard::Impl::createEmptyCardFile(const std::string& fname, uint64_t totalBytes)
{
	std::ofstream ofs(fname, std::ios::binary | std::ios::out);
	if (!ofs)
	{
		return m_error.Fail("Could not create file: " + fname);
	}

	std::vector<char> chunk(65536, 0);
	uint64_t written = 0;
	while (written < totalBytes)
	{
		uint64_t toWrite = (std::min)(static_cast<uint64_t>(chunk.size()), totalBytes - written);
		ofs.write(chunk.data(), toWrite);
		if (!ofs)
		{
			return m_error.Fail("Could not create file: " + fname);
		}
		written += toWrite;
	}
	return true;
}

bool PS2MemoryCard::Impl::writeSuperblock(uint32_t first_ifc, uint32_t indirect_fat_clusters, uint32_t good_block1, uint32_t good_block2)
{
	std::vector<uint8_t> sb(m_pageSize, 0);
	std::memcpy(sb.data(), "Sony PS2 Memory Card Format ", 28);
	std::memcpy(sb.data() + 28, "1.2.0.0", 8);

	writeLE<uint16_t>(sb, 40, static_cast<uint16_t>(m_pageSize));
	writeLE<uint16_t>(sb, 42, static_cast<uint16_t>(m_pagesPerCluster));
	writeLE<uint16_t>(sb, 44, static_cast<uint16_t>(m_pagesPerEraseBlock));
	writeLE<uint16_t>(sb, 46, 0xFF00);
	writeLE<uint32_t>(sb, 48, m_clustersPerCard);
	writeLE<uint32_t>(sb, 52, m_allocatableClusterOffset);
	writeLE<uint32_t>(sb, 56, m_allocatableClusterEnd);
	writeLE<uint32_t>(sb, 60, m_rootDirFatCluster);
	writeLE<uint32_t>(sb, 64, good_block1);
	writeLE<uint32_t>(sb, 68, good_block2);

	for (uint32_t i = 0; i < indirect_fat_clusters; ++i)
	{
		uint32_t ifc_phys = first_ifc + i;
		m_indirectFatClusterList[i] = ifc_phys;
		writeLE<uint32_t>(sb, 80 + i * 4, ifc_phys);
	}

	for (int i = 0; i < 32; ++i)
	{
		writeLE<uint32_t>(sb, 208 + i * 4, 0xFFFFFFFF);
	}

	sb[336] = 2;
	sb[337] = m_withEcc ? 0x2B : 0x2A;
	m_cardType = 2;
	m_cardFlags = sb[337];

	if (!writePage(0, sb))
	{
		return false;
	}

	const uint64_t backup2_offset = static_cast<uint64_t>(good_block2) * m_pagesPerEraseBlock * m_rawPageSize;
	std::vector<uint8_t> erased_page(m_rawPageSize, 0xFF);
	m_file.seekp(static_cast<std::streamoff>(backup2_offset));
	for (uint32_t i = 0; i < m_pagesPerEraseBlock; ++i)
	{
		m_file.write(reinterpret_cast<const char*>(erased_page.data()),
			static_cast<std::streamsize>(erased_page.size()));
		if (!m_file)
		{
			return m_error.Fail("Failed to initialize backup erase block");
		}
	}
	return true;
}

bool PS2MemoryCard::Impl::initIndirectFatClusters(uint32_t first_ifc, uint32_t indirect_fat_clusters, uint32_t epc)
{
	uint32_t current_fat_cluster_phys = first_ifc + indirect_fat_clusters;

	for (uint32_t i = 0; i < indirect_fat_clusters; ++i)
	{
		uint32_t ifc_phys = first_ifc + i;
		std::vector<uint8_t> ifc_data(m_clusterSize, 0);

		for (uint32_t j = 0; j < epc; ++j)
		{
			if (current_fat_cluster_phys < m_allocatableClusterOffset)
			{
				writeLE<uint32_t>(ifc_data, j * 4, current_fat_cluster_phys++);
			}
			else
			{
				writeLE<uint32_t>(ifc_data, j * 4, 0xFFFFFFFF);
			}
		}
		if (!writeCluster(ifc_phys, ifc_data))
		{
			return false;
		}
	}
	return true;
}

bool PS2MemoryCard::Impl::initRootDirectory()
{
	m_fat.clear();
	m_fat.resize(m_allocatableClusterEnd, PS2MC_FAT_CHAIN_END_UNALLOC);
	m_fat[0] = PS2MC_FAT_CHAIN_END;

	if (!writeFatToCard())
	{
		return false;
	}

	std::vector<PS2McDirEntry> rootEntries;

	PS2McDirEntry dot{};
	dot.mode = DF_DIR | DF_EXISTS | DF_RWX | DF_0400;
	dot.name = ".";
	dot.cluster = 0;
	dot.length = 2;
	dot.created = timeToTod(time(nullptr));
	dot.modified = dot.created;
	rootEntries.push_back(dot);

	PS2McDirEntry dotdot{};
	dotdot.mode = DF_DIR | DF_EXISTS | DF_WRITE | DF_EXECUTE | DF_0400 | DF_HIDDEN;
	dotdot.name = "..";
	dotdot.cluster = 0;
	dotdot.length = 0;
	dotdot.created = dot.created;
	dotdot.modified = dot.modified;
	rootEntries.push_back(dotdot);

	return writeDirents(0, rootEntries);
}

bool PS2MemoryCard::create(const std::string& filename, int sizeInMB, bool disableEcc)
{
	m_error.Clear();
	close();

	m_impl->m_filename = filename;
	m_impl->initCardParameters(sizeInMB, disableEcc);

	uint32_t first_ifc, indirect_fat_clusters, fat_clusters;
	m_impl->calculateFatLayout(first_ifc, indirect_fat_clusters, fat_clusters);

	uint32_t pages_per_card = m_impl->m_clustersPerCard * m_impl->m_pagesPerCluster;
	uint32_t erase_blocks_per_card = pages_per_card / m_impl->m_pagesPerEraseBlock;
	uint32_t good_block1 = erase_blocks_per_card - 1;
	uint32_t good_block2 = erase_blocks_per_card - 2;

	uint64_t totalBytes = static_cast<uint64_t>(m_impl->m_clustersPerCard) *
	                      m_impl->m_pagesPerCluster * m_impl->m_rawPageSize;

	if (!m_impl->createEmptyCardFile(filename, totalBytes))
	{
		return false;
	}

	m_impl->m_file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
	if (!m_impl->m_file)
	{
		return m_error.Fail("Failed to open created card");
	}

	if (!m_impl->writeSuperblock(first_ifc, indirect_fat_clusters, good_block1, good_block2))
	{
		return false;
	}

	uint32_t epc = m_impl->m_clusterSize / 4;
	if (!m_impl->initIndirectFatClusters(first_ifc, indirect_fat_clusters, epc))
	{
		return false;
	}

	return m_impl->initRootDirectory();
}

std::vector<PS2McDirEntry> PS2MemoryCard::listDir(const std::string& path)
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		m_error.Fail("Memory card not open");
		return {};
	}

	std::string check_path = path.empty() ? "/" : path;

	if (check_path == "/")
	{
		return m_impl->readDirents(m_impl->m_rootDirFatCluster);
	}

	uint32_t parent_cluster = 0;
	PS2McDirEntry entry{};
	if (!m_impl->findEntry(check_path, parent_cluster, entry))
	{
		return {};
	}

	if (!(entry.mode & DF_DIR))
	{
		m_error.Fail("Not a directory: " + path);
		return {};
	}

	return m_impl->readDirents(entry.cluster);
}

bool PS2MemoryCard::hasEntry(const std::string& path)
{
	if (!m_impl || !m_impl->m_file.is_open())
	{
		return false;
	}

	std::string check_path = path.empty() ? "/" : path;
	if (check_path == "/")
	{
		return true;
	}

	uint32_t parent_cluster = 0;
	PS2McDirEntry entry{};
	bool path_not_found = false;
	if (m_impl->findEntry(check_path, parent_cluster, entry, &path_not_found, /*quiet=*/true))
	{
		return (entry.mode & DF_EXISTS) != 0;
	}
	return false;
}

PS2McDirEntry PS2MemoryCard::getEntry(const std::string& path)
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		m_error.Fail("Memory card not open");
		return {};
	}

	if (path == "/")
	{
		PS2McDirEntry root{};
		root.mode = DF_DIR | DF_EXISTS;
		root.name = "/";
		root.cluster = m_impl->m_rootDirFatCluster;
		return root;
	}

	uint32_t parent_cluster = 0;
	PS2McDirEntry entry{};
	m_impl->findEntry(path, parent_cluster, entry);
	return entry;
}

std::vector<uint8_t> PS2MemoryCard::readFile(const std::string& path, bool quiet)
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		if (!quiet)
		{
			m_error.Fail("Memory card not open");
		}
		return {};
	}

	uint32_t parent_cluster = 0;
	PS2McDirEntry entry{};
	if (!m_impl->findEntry(path, parent_cluster, entry, nullptr, quiet))
	{
		return {};
	}

	if (entry.mode & DF_DIR)
	{
		if (!quiet)
		{
			m_error.Fail("Cannot read directory as file");
		}
		return {};
	}

	std::vector<uint8_t> result;
	result.reserve(entry.length);

	// Follow the FAT chain from entry.cluster
	uint32_t current_cluster = entry.cluster;
	uint32_t bytes_read = 0;

	while (bytes_read < entry.length && current_cluster != PS2MC_FAT_CHAIN_END && current_cluster < m_impl->m_fat.size())
	{
		uint32_t disk_cluster = current_cluster + m_impl->m_allocatableClusterOffset;
		auto cluster_data = m_impl->readCluster(disk_cluster);
		if (cluster_data.empty())
		{
			return {};
		}
		uint32_t remaining = entry.length - bytes_read;
		uint32_t to_read = remaining < m_impl->m_clusterSize ? remaining : m_impl->m_clusterSize;

		result.insert(result.end(),
			cluster_data.begin(),
			cluster_data.begin() + to_read);
		bytes_read += to_read;

		uint32_t next = m_impl->m_fat[current_cluster] & PS2MC_FAT_CLUSTER_MASK;
		if (next == PS2MC_FAT_CHAIN_END_UNALLOC)
		{
			break;
		}
		current_cluster = next;
	}

	if (bytes_read != entry.length)
	{
		m_error.Fail("File data is truncated: " + path);
		return {};
	}
	return result;
}

bool PS2MemoryCard::exportFile(const std::string& path, const std::string& dest_path)
{
	auto data = readFile(path);
	if (m_error.IsValid())
	{
		return false;
	}
	std::ofstream outfile(dest_path, std::ios::binary);
	if (!outfile)
	{
		return m_error.Fail("Failed to create export file: " + dest_path);
	}
	outfile.write(reinterpret_cast<const char*>(data.data()), data.size());
	if (!outfile)
	{
		return m_error.Fail("Failed to write export file: " + dest_path);
	}
	return true;
}

std::vector<uint8_t> PS2MemoryCard::getIconData(const std::string& savePath)
{
	Logger::debug("getIconData: Trying to load icon for path: {}", savePath);

	std::string iconSysPath = savePath + "/icon.sys";
	Logger::debug("getIconData: Trying to read icon.sys at: {}", iconSysPath);
	auto iconSysData = readFile(iconSysPath, /*quiet=*/true);
	if (!iconSysData.empty())
	{
		Logger::debug("getIconData: icon.sys loaded, size: {}", iconSysData.size());
		PS2IconSys iconSys;
		iconSys.load(iconSysData);

		std::string iconFile = iconSys.getIconFileNormal();
		Logger::debug("getIconData: icon.sys specifies icon file: {}", iconFile);
		if (!iconFile.empty())
		{
			std::string iconPath = savePath + "/" + iconFile;
			Logger::debug("getIconData: Trying to read icon at: {}", iconPath);
			auto data = readFile(iconPath, /*quiet=*/true);
			if (!data.empty())
			{
				Logger::debug("getIconData: Successfully loaded icon, size: {}", data.size());
				m_error.Clear();
				return data;
			}
		}
	}

	std::vector<std::string> iconPaths = {
		savePath + "/icon0.icn",
		savePath + "/icon1.icn",
		savePath + "/icon.icn"};

	for (const auto& iconPath : iconPaths)
	{
		auto data = readFile(iconPath, /*quiet=*/true);
		if (!data.empty())
		{
			m_error.Clear();
			return data;
		}
	}

	m_error.Clear();
	return std::vector<uint8_t>();
}

PS2IconSys* PS2MemoryCard::getIconSys(const std::string& savePath)
{
	std::string iconSysPath = savePath + "/icon.sys";
	auto iconSysData = readFile(iconSysPath, /*quiet=*/true);
	m_error.Clear();

	if (iconSysData.empty())
	{
		return nullptr;
	}

	auto* iconSys = new PS2IconSys();
	iconSys->load(iconSysData);

	return iconSys;
}

std::string PS2MemoryCard::getSaveTitle(const std::string& savePath)
{
	auto iconData = readFile(savePath + "/icon.sys", /*quiet=*/true);
	m_error.Clear();
	if (iconData.empty())
	{
		return "";
	}

	PS2IconSys iconSys;
	iconSys.load(iconData);
	return iconSys.getTitle();
}

std::string PS2MemoryCard::getSaveSubtitle(const std::string& savePath)
{
	auto iconData = readFile(savePath + "/icon.sys", /*quiet=*/true);
	m_error.Clear();
	if (iconData.empty())
	{
		return "";
	}

	PS2IconSys iconSys;
	iconSys.load(iconData);
	return iconSys.getSubtitle();
}

uint32_t PS2MemoryCard::getSaveSize(const std::string& savePath)
{
	auto entries = listDir(savePath);
	m_error.Clear();
	if (entries.empty())
	{
		return 0;
	}

	uint32_t totalSize = roundUp(static_cast<uint32_t>(entries.size()) * PS2MC_DIRENT_LENGTH, m_impl->m_clusterSize);

	for (const auto& entry : entries)
	{
		if (entry.mode & DF_HIDDEN || entry.name == "." || entry.name == "..")
		{
			continue;
		}

		if (entry.mode & DF_DIR)
		{
			std::string subPath = savePath + "/" + entry.name;
			totalSize += getSaveSize(subPath);
		}
		else
		{
			totalSize += roundUp(entry.length, m_impl->m_clusterSize);
		}
	}

	return totalSize;
}

uint32_t PS2MemoryCard::getFreeSpace()
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		m_error.Fail("Memory card not open");
		return 0;
	}
	uint32_t free_clusters = 0;
	for (uint32_t entry : m_impl->m_fat)
	{
		if ((entry & PS2MC_FAT_ALLOCATED_BIT) == 0 && entry == PS2MC_FAT_CHAIN_END_UNALLOC)
		{
			free_clusters++;
		}
	}
	return free_clusters * m_impl->m_clusterSize;
}

PS2MemoryCard::CardInfo PS2MemoryCard::getCardInfo()
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		m_error.Fail("Memory card not open");
		return {};
	}

	CardInfo info{};

	info.pageSize = m_impl->m_pageSize;
	info.rawPageSize = m_impl->m_rawPageSize;
	info.spareSize = m_impl->m_spareSize;
	info.withEcc = m_impl->m_withEcc && m_impl->m_spareSize > 0;
	info.cardType = m_impl->m_cardType;
	info.cardFlags = m_impl->m_cardFlags;
	info.pagesPerCluster = m_impl->m_pagesPerCluster;
	info.clusterSize = m_impl->m_clusterSize;
	info.clustersPerCard = m_impl->m_clustersPerCard;
	info.allocatableOffset = m_impl->m_allocatableClusterOffset;
	info.allocatableCount = m_impl->m_allocatableClusterEnd;
	info.rootDirCluster = m_impl->m_rootDirFatCluster;

	uint32_t freeClusters = 0;
	for (uint32_t entry : m_impl->m_fat)
	{
		if ((entry & PS2MC_FAT_ALLOCATED_BIT) == 0 && entry == PS2MC_FAT_CHAIN_END_UNALLOC)
		{
			freeClusters++;
		}
	}

	info.freeClusters = freeClusters;

	uint32_t allocatableClusters = info.allocatableCount;
	uint32_t usedClusters = (allocatableClusters > freeClusters) ? (allocatableClusters - freeClusters) : 0;
	info.usedClusters = usedClusters;

	uint32_t reservedClusters = 0;
	if (m_impl->m_clustersPerCard > info.allocatableOffset + info.allocatableCount)
	{
		reservedClusters = m_impl->m_clustersPerCard - (info.allocatableOffset + info.allocatableCount);
	}
	info.reservedClusters = reservedClusters;

	info.backupBlock1 = m_impl->m_backupBlock1;
	info.backupBlock2 = m_impl->m_backupBlock2;

	uint32_t badCount = 0;
	for (uint32_t v : m_impl->m_badBlockList)
	{
		if (v != 0 && v != 0xFFFFFFFF)
		{
			++badCount;
		}
	}
	info.badBlockCount = badCount;

	info.usedBytes = static_cast<uint64_t>(usedClusters) * info.clusterSize;
	info.freeBytes = static_cast<uint64_t>(freeClusters) * info.clusterSize;

	info.imageSizeBytes =
		static_cast<uint64_t>(m_impl->m_pageSize) * m_impl->m_pagesPerCluster * m_impl->m_clustersPerCard;

	return info;
}

bool PS2MemoryCard::makeDir(const std::string& path)
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		return m_error.Fail("Memory card not open");
	}

	if (path == "/" || path.empty())
	{
		return m_error.Fail("Invalid directory path");
	}

	uint32_t unusedParent = 0;
	PS2McDirEntry existingEntry{};
	bool path_not_found = false;
	if (m_impl->findEntry(path, unusedParent, existingEntry, &path_not_found, /*quiet=*/true))
	{
		if (existingEntry.mode & DF_EXISTS)
		{
			return true;
		}
	}
	else if (!path_not_found)
	{
		return false;
	}
	m_error.Clear();

	size_t last_slash = path.rfind('/');
	std::string parent_path = (last_slash == 0) ? "/" : path.substr(0, last_slash);
	std::string dir_name = path.substr(last_slash + 1);

	uint32_t parent_cluster = 0;
	if (parent_path != "/")
	{
		PS2McDirEntry parent_entry{};
		if (!m_impl->findEntry(parent_path, parent_cluster, parent_entry))
		{
			return false;
		}
		parent_cluster = parent_entry.cluster;
	}
	else
	{
		parent_cluster = m_impl->m_rootDirFatCluster;
	}

	uint32_t dir_cluster = m_impl->allocateCluster();
	if (dir_cluster == PS2MC_FAT_CHAIN_END)
	{
		return false;
	}

	auto parent_entries = m_impl->readDirents(parent_cluster);
	if (m_error.IsValid())
	{
		return false;
	}
	const uint32_t slot_for_new_dir = static_cast<uint32_t>(parent_entries.size());

	const auto now = timeToTod(time(nullptr));

	PS2McDirEntry new_dir_entry = {};
	new_dir_entry.mode = DF_DIR | DF_EXISTS | DF_RWX | DF_0400;
	new_dir_entry.name = dir_name;
	new_dir_entry.cluster = dir_cluster;
	new_dir_entry.length = 2;
	new_dir_entry.created = now;
	new_dir_entry.modified = now;

	std::vector<PS2McDirEntry> new_dir_entries;

	PS2McDirEntry dot_entry = {};
	dot_entry.mode = DF_DIR | DF_EXISTS | DF_RWX | DF_0400;
	dot_entry.name = ".";
	dot_entry.cluster = parent_cluster;
	dot_entry.length = 2;
	dot_entry.created = now;
	dot_entry.modified = now;
	dot_entry.dirEntry = slot_for_new_dir;
	new_dir_entries.push_back(dot_entry);

	PS2McDirEntry dotdot_entry = {};
	dotdot_entry.mode = DF_DIR | DF_EXISTS | DF_RWX | DF_0400;
	dotdot_entry.name = "..";
	dotdot_entry.cluster = 0;
	dotdot_entry.length = 0;
	dotdot_entry.created = now;
	dotdot_entry.modified = now;
	dotdot_entry.dirEntry = 0;
	new_dir_entries.push_back(dotdot_entry);

	if (!m_impl->writeDirents(dir_cluster, new_dir_entries))
	{
		return false;
	}

	parent_entries.push_back(new_dir_entry);
	if (!parent_entries.empty() && (parent_entries[0].mode & DF_DIR))
	{
		parent_entries[0].length = static_cast<uint32_t>(parent_entries.size());
	}

	if (!m_impl->writeDirents(parent_cluster, parent_entries))
	{
		return false;
	}

	if (!m_impl->writeFatToCard())
	{
		return false;
	}

	m_impl->m_modified = true;
	return true;
}

bool PS2MemoryCard::writeFile(const std::string& path, const std::vector<uint8_t>& data)
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		return m_error.Fail("Memory card not open");
	}

	if (path == "/" || path.empty())
	{
		return m_error.Fail("Invalid file path");
	}

	uint32_t unusedParent = 0;
	PS2McDirEntry existingEntry{};
	bool path_not_found = false;
	if (m_impl->findEntry(path, unusedParent, existingEntry, &path_not_found, /*quiet=*/true) && (existingEntry.mode & DF_EXISTS))
	{
		return m_error.Fail("File already exists: " + path);
	}
	if (!path_not_found)
	{
		return false;
	}
	m_error.Clear();

	size_t last_slash = path.rfind('/');
	std::string parent_path = (last_slash == 0) ? "/" : path.substr(0, last_slash);
	std::string file_name = path.substr(last_slash + 1);

	uint32_t parent_cluster = 0;
	if (parent_path != "/")
	{
		PS2McDirEntry parent_entry{};
		if (!m_impl->findEntry(parent_path, parent_cluster, parent_entry))
		{
			return false;
		}
		parent_cluster = parent_entry.cluster;
	}
	else
	{
		parent_cluster = m_impl->m_rootDirFatCluster;
	}

	// Calculate clusters needed
	uint32_t clusters_needed = (static_cast<uint32_t>(data.size()) + m_impl->m_clusterSize - 1) / m_impl->m_clusterSize;
	std::vector<uint32_t> file_clusters;
	if (clusters_needed > 0)
	{
		file_clusters = m_impl->allocateClusters(clusters_needed);
		if (file_clusters.empty())
		{
			return false;
		}
	}

	uint32_t offset = 0;
	for (uint32_t cluster : file_clusters)
	{
		uint32_t remaining = static_cast<uint32_t>(data.size()) - offset;
		uint32_t to_write = (m_impl->m_clusterSize < remaining) ? m_impl->m_clusterSize : remaining;
		std::vector<uint8_t> cluster_data(m_impl->m_clusterSize, 0);

		if (to_write > 0)
		{
			std::copy(data.begin() + offset, data.begin() + offset + to_write, cluster_data.begin());
		}

		uint32_t disk_cluster = cluster + m_impl->m_allocatableClusterOffset;
		if (!m_impl->writeCluster(disk_cluster, cluster_data))
		{
			return false;
		}

		offset += to_write;
	}

	PS2McDirEntry file_entry = {};
	file_entry.mode = DF_FILE | DF_EXISTS | DF_RWX | DF_0400;
	file_entry.name = file_name;
	file_entry.cluster = file_clusters.empty() ? PS2MC_FAT_CHAIN_END : file_clusters[0];
	file_entry.length = static_cast<uint32_t>(data.size());
	file_entry.created = timeToTod(time(nullptr));
	file_entry.modified = file_entry.created;

	auto parent_entries = m_impl->readDirents(parent_cluster);
	if (m_error.IsValid())
	{
		return false;
	}
	parent_entries.push_back(file_entry);
	if (!parent_entries.empty() && (parent_entries[0].mode & DF_DIR) && parent_entries[0].name == ".")
	{
		parent_entries[0].length = static_cast<uint32_t>(parent_entries.size());
	}
	if (!m_impl->writeDirents(parent_cluster, parent_entries) ||
		!m_impl->syncParentDirectoryEntryLength(parent_cluster))
	{
		return false;
	}

	if (!m_impl->writeFatToCard())
	{
		return false;
	}

	m_impl->m_modified = true;
	return true;
}

bool PS2MemoryCard::remove(const std::string& path)
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		return m_error.Fail("Memory card not open");
	}

	if (path == "/" || path.empty())
	{
		return m_error.Fail("Cannot delete root directory");
	}

	uint32_t parent_cluster = 0;
	PS2McDirEntry entry{};
	if (!m_impl->findEntry(path, parent_cluster, entry))
	{
		return false;
	}
	if (!(entry.mode & DF_EXISTS))
	{
		return m_error.Fail("File not found: " + path);
	}

	entry.mode &= ~DF_EXISTS;

	if (entry.mode & DF_DIR)
	{
		auto contents = listDir(path);
		if (m_error.IsValid())
		{
			return false;
		}
		for (const auto& sub : contents)
		{
			if (sub.name == "." || sub.name == "..")
			{
				continue;
			}
			if (sub.mode & DF_EXISTS)
			{
				if (!remove(path + "/" + sub.name))
				{
					return false;
				}
			}
		}

		uint32_t cluster = entry.cluster;
		while (cluster != PS2MC_FAT_CHAIN_END && cluster < m_impl->m_fat.size())
		{
			uint32_t next = m_impl->m_fat[cluster] & PS2MC_FAT_CLUSTER_MASK;
			m_impl->m_fat[cluster] = PS2MC_FAT_CHAIN_END_UNALLOC;
			cluster = next;
		}
	}
	else
	{
		uint32_t cluster = entry.cluster;
		while (cluster != PS2MC_FAT_CHAIN_END && cluster < m_impl->m_fat.size())
		{
			uint32_t next = m_impl->m_fat[cluster] & PS2MC_FAT_CLUSTER_MASK;
			m_impl->m_fat[cluster] = PS2MC_FAT_CHAIN_END_UNALLOC;
			cluster = next;
		}
	}

	auto parent_entries = m_impl->readDirents(parent_cluster);
	if (m_error.IsValid())
	{
		return false;
	}
	for (auto& e : parent_entries)
	{
		if (e.name == entry.name)
		{
			e.mode = entry.mode; // Update with deleted flag
			break;
		}
	}

	if (!m_impl->writeDirents(parent_cluster, parent_entries))
	{
		return false;
	}

	if (!m_impl->writeFatToCard())
	{
		return false;
	}
	m_impl->m_file.flush();
	if (!m_impl->m_file)
	{
		return m_error.Fail("Failed to flush memory card file");
	}

	m_impl->m_modified = true;
	return true;
}

bool PS2MemoryCard::importSaveFile(PS2SaveFile& save, bool ignoreExisting, const std::string& targetDir)
{
	m_error.Clear();
	if (!m_impl || !m_impl->m_file.is_open())
	{
		return m_error.Fail("No memory card open");
	}

	const auto& entries = save.getEntries();
	if (entries.empty())
	{
		return m_error.Fail("Save file contains no entries");
	}

	const bool hasDirHeader = (entries[0].dirEntry.mode & DF_DIR) != 0;
	PS2McDirEntry dirEntry{};
	if (hasDirHeader)
	{
		dirEntry = entries[0].dirEntry;
	}
	else
	{
		dirEntry.name = save.getTitle();
		if (dirEntry.name.empty())
		{
			dirEntry.name = entries[0].dirEntry.name;
		}
		dirEntry.mode = DF_DIR | DF_EXISTS | DF_RWX | DF_0400;
		dirEntry.created = entries[0].dirEntry.created;
		dirEntry.modified = dirEntry.created;
		dirEntry.unused = 0;
		dirEntry.attr = 0;
	}

	std::string saveDirName = targetDir.empty() ? dirEntry.name : targetDir;

	if (!saveDirName.empty() && saveDirName[0] == '/')
	{
		saveDirName = saveDirName.substr(1);
	}

	std::string savePath = "/" + saveDirName;

	uint32_t dummy = 0;
	PS2McDirEntry checkExisting{};
	bool path_not_found = false;
	if (m_impl->findEntry(savePath, dummy, checkExisting, &path_not_found, /*quiet=*/true))
	{
		if (!ignoreExisting)
		{
			return false; // Save already exists, not imported
		}

		if (!remove(savePath))
		{
			return false;
		}
	}
	else if (!path_not_found)
	{
		return false;
	}
	m_error.Clear();

	if (!makeDir(savePath))
	{
		return false;
	}

	const size_t firstFileIdx = hasDirHeader ? 1 : 0;
	for (size_t i = firstFileIdx; i < entries.size(); ++i)
	{
		const auto& entry = entries[i];
		std::string filePath = savePath + "/" + entry.dirEntry.name;

		if (!writeFile(filePath, entry.data))
		{
			return false;
		}

		uint32_t parent_cluster_inner = 0;
		PS2McDirEntry currentEntry{};
		if (!m_impl->findEntry(filePath, parent_cluster_inner, currentEntry))
		{
			return false;
		}
		else
		{
			auto parentEntries = m_impl->readDirents(parent_cluster_inner);
			if (m_error.IsValid())
			{
				return false;
			}
			bool updated = false;
			for (auto& pe : parentEntries)
			{
				if (pe.name == currentEntry.name)
				{
					const uint16_t psuMode = entry.dirEntry.mode;
					constexpr uint16_t typeMask = DF_FILE | DF_DIR | DF_EXISTS;
					pe.mode = static_cast<uint16_t>((psuMode & ~typeMask) | (pe.mode & typeMask));
					if ((pe.mode & DF_EXISTS) && (pe.mode & (DF_FILE | DF_DIR)) && !(pe.mode & DF_PSX))
					{
						pe.mode |= DF_0400;
					}
					pe.unused = entry.dirEntry.unused;
					pe.created = entry.dirEntry.created;
					pe.modified = entry.dirEntry.modified;
					pe.attr = entry.dirEntry.attr;
					updated = true;
					break;
				}
			}
			if (updated)
			{
				if (!m_impl->writeDirents(parent_cluster_inner, parentEntries) ||
					!m_impl->syncParentDirectoryEntryLength(parent_cluster_inner))
				{
					return false;
				}
			}
		}
	}

	{
		uint32_t saveParentCluster = 0;
		PS2McDirEntry unusedEntry{};
		if (!m_impl->findEntry(savePath, saveParentCluster, unusedEntry))
		{
			return false;
		}

		auto parentRows = m_impl->readDirents(saveParentCluster);
		if (m_error.IsValid())
		{
			return false;
		}
		constexpr uint16_t typeMask = DF_FILE | DF_DIR | DF_EXISTS;
		for (auto& row : parentRows)
		{
			if (row.name == saveDirName && (row.mode & DF_DIR))
			{
				row.mode = static_cast<uint16_t>((dirEntry.mode & ~typeMask) | (row.mode & typeMask));
				if ((row.mode & DF_EXISTS) && (row.mode & DF_DIR) && !(row.mode & DF_PSX))
				{
					row.mode |= DF_0400;
				}
				row.unused = dirEntry.unused;
				row.created = dirEntry.created;
				row.modified = dirEntry.modified;
				row.attr = dirEntry.attr;
				break;
			}
		}
		if (!m_impl->writeDirents(saveParentCluster, parentRows))
		{
			return false;
		}
	}

	uint32_t rootParent = m_impl->m_rootDirFatCluster;
	auto finalRoot = m_impl->readDirents(rootParent);
	if (m_error.IsValid())
	{
		return false;
	}
	if (!finalRoot.empty() && (finalRoot[0].mode & DF_DIR))
	{
		finalRoot[0].length = static_cast<uint32_t>(finalRoot.size());
		if (!m_impl->writeDirents(rootParent, finalRoot))
		{
			return false;
		}
	}

	if (!m_impl->writeFatToCard())
	{
		return false;
	}
	m_impl->m_file.flush();
	if (!m_impl->m_file)
	{
		return m_error.Fail("Failed to flush memory card file");
	}

	return true;
}

bool PS2MemoryCard::exportSaveFile(const std::string& savePath, PS2SaveFile& save)
{
	m_error.Clear();
	if (!m_impl || !m_impl->m_file.is_open())
	{
		return m_error.Fail("No memory card open");
	}

	// Read the save directory entry
	uint32_t parent_cluster = 0;
	PS2McDirEntry dirEntry{};
	if (!m_impl->findEntry(savePath, parent_cluster, dirEntry))
	{
		return false;
	}

	if (!(dirEntry.mode & DF_DIR))
	{
		return m_error.Fail("Path is not a directory");
	}

	// Create PS2SaveEntry for the directory itself
	PS2SaveEntry dirSaveEntry;
	dirSaveEntry.dirEntry = dirEntry;

	auto& entries = save.getEntries();
	entries.clear();
	entries.push_back(dirSaveEntry);

	// Read all files in the directory
	auto dirContents = m_impl->readDirents(dirEntry.cluster);
	if (m_error.IsValid())
	{
		return false;
	}

	for (const auto& entry : dirContents)
	{
		if (entry.name == "." || entry.name == "..")
		{
			continue;
		}

		if (!(entry.mode & DF_EXISTS))
		{
			continue;
		}

		if (entry.mode & DF_DIR)
		{
			continue;
		}

		PS2SaveEntry fileEntry;
		fileEntry.dirEntry = entry;

		std::string filePath = savePath + "/" + entry.name;
		fileEntry.data = readFile(filePath);
		if (m_error.IsValid())
		{
			return false;
		}

		entries.push_back(fileEntry);
	}

	// Set the save title from icon.sys if available
	std::string title = getSaveTitle(savePath);
	if (!title.empty())
	{
		save.setTitle(title);
	}

	return true;
}

uint16_t PS2MemoryCard::getMode(const std::string& path)
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		m_error.Fail("Memory card not open");
		return 0;
	}

	uint32_t parent_cluster = 0;
	PS2McDirEntry entry{};
	if (!m_impl->findEntry(path, parent_cluster, entry))
	{
		return 0;
	}
	return entry.mode;
}

bool PS2MemoryCard::setMode(const std::string& path, uint16_t mode)
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		return m_error.Fail("Memory card not open");
	}

	uint32_t parent_cluster = 0;
	PS2McDirEntry entry{};
	if (!m_impl->findEntry(path, parent_cluster, entry))
	{
		return false;
	}

	// Read all entries in the parent directory
	auto entries = m_impl->readDirents(parent_cluster);
	if (m_error.IsValid())
	{
		return false;
	}

	// Find and update the target entry
	bool found = false;
	for (auto& e : entries)
	{
		if (e.name == entry.name && e.dirEntry == entry.dirEntry)
		{
			e.mode = mode;
			found = true;
			break;
		}
	}

	if (!found)
	{
		return m_error.Fail("Entry not found in parent directory");
	}

	// Write all entries back
	return m_impl->writeDirents(parent_cluster, entries);
}

bool PS2MemoryCard::setModifiedTime(const std::string& path, std::time_t newTime)
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		return m_error.Fail("Memory card not open");
	}

	uint32_t parent_cluster = 0;
	PS2McDirEntry entry{};
	if (!m_impl->findEntry(path, parent_cluster, entry))
	{
		return false;
	}

	auto entries = m_impl->readDirents(parent_cluster);
	if (m_error.IsValid())
	{
		return false;
	}
	PS2McTod newTod = timeToTod(newTime);

	bool found = false;
	for (auto& e : entries)
	{
		if (e.name == entry.name && e.dirEntry == entry.dirEntry)
		{
			e.modified = newTod;
			found = true;
			break;
		}
	}

	if (!found)
	{
		return m_error.Fail("Entry not found in parent directory");
	}

	if (!m_impl->writeDirents(parent_cluster, entries))
	{
		return false;
	}
	m_impl->m_modified = true;
	return true;
}

bool PS2MemoryCard::renameEntry(const std::string& path, const std::string& newName)
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		return m_error.Fail("Memory card not open");
	}

	if (path.empty() || path == "/")
	{
		return m_error.Fail("Invalid path for rename");
	}

	if (newName.empty() || newName == "." || newName == "..")
	{
		return m_error.Fail("Invalid new name");
	}

	if (newName.find('/') != std::string::npos)
	{
		return m_error.Fail("Name cannot contain '/'");
	}

	uint32_t parent_cluster = 0;
	PS2McDirEntry entry{};
	if (!m_impl->findEntry(path, parent_cluster, entry))
	{
		return false;
	}

	auto entries = m_impl->readDirents(parent_cluster);
	if (m_error.IsValid())
	{
		return false;
	}

	for (const auto& e : entries)
	{
		if ((e.mode & DF_EXISTS) && e.name == newName)
		{
			return m_error.Fail("An entry with the new name already exists");
		}
	}

	bool found = false;
	for (auto& e : entries)
	{
		if (e.name == entry.name && e.dirEntry == entry.dirEntry)
		{
			e.name = newName;
			found = true;
			break;
		}
	}

	if (!found)
	{
		return m_error.Fail("Entry not found in parent directory");
	}

	if (!m_impl->writeDirents(parent_cluster, entries))
	{
		return false;
	}
	m_impl->m_modified = true;
	return true;
}

uint32_t PS2MemoryCard::getAllocatableSpace()
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		m_error.Fail("Memory card not open");
		return 0;
	}

	// Return total usable space on card (excluding system clusters)
	// This is the total clusters available for user data
	uint32_t totalClusters = m_impl->m_clustersPerCard;
	uint32_t systemClusters = m_impl->m_allocatableClusterOffset; // Clusters reserved for FAT, etc.
	return (totalClusters - systemClusters) * m_impl->m_clusterSize;
}

bool PS2MemoryCard::check()
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		return m_error.Fail("Memory card not open");
	}

	auto entries = listDir("/");
	if (m_error.IsValid())
	{
		return false;
	}
	for (const auto& entry : entries)
	{
		if (entry.mode & DF_DIR)
		{
			auto subentries = listDir("/" + entry.name);
			if (m_error.IsValid())
			{
				return false;
			}
			for (const auto& subentry : subentries)
			{
				if (subentry.mode & DF_FILE)
				{
					std::string path = "/" + entry.name + "/" + subentry.name;
					auto data = readFile(path);
					if (m_error.IsValid())
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}

bool PS2MemoryCard::hasEcc() const
{
	return m_impl->m_withEcc;
}

std::string PS2MemoryCard::getPsxTitle(const std::string& savePath)
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		m_error.Fail("Memory card not open");
		return "";
	}

	// Get the save directory mode to check if it's PSX
	auto entry = getEntry(savePath);
	if (!(entry.mode & DF_PSX))
	{
		m_error.Clear();
		return ""; // Not a PSX save
	}

	// Look for the first file in the save directory
	auto entries = listDir(savePath);
	for (const auto& e : entries)
	{
		if (e.mode & DF_FILE)
		{
			// Read the PSX save header (first 128 bytes)
			std::string filePath = savePath + "/" + e.name;
			auto data = readFile(filePath);
			m_error.Clear();

			if (data.size() >= 128)
			{
				if (data[0] == 'S' && data[1] == 'C')
				{
					std::string title(reinterpret_cast<const char*>(&data[4]), 64);
					size_t nullPos = title.find('\0');
					if (nullPos != std::string::npos)
					{
						title = title.substr(0, nullPos);
					}
					return ShiftJIS::toUtf8(title);
				}
			}
			break; // Only check first file
		}
	}

	m_error.Clear();
	return "";
}

bool PS2MemoryCard::saveAs(const std::string& filename, bool withEcc)
{
	m_error.Clear();
	if (!m_impl->m_file.is_open())
	{
		return m_error.Fail("Memory card not open");
	}

	std::error_code ec;
	bool sameFile = std::filesystem::equivalent(
		std::filesystem::path(m_impl->m_filename), std::filesystem::path(filename), ec);
	if (ec)
	{
		sameFile = false;
	}

	if (m_impl->m_modified)
	{
		if (!m_impl->writeFatToCard())
		{
			return false;
		}
	}
	m_impl->m_file.flush();
	if (!m_impl->m_file)
	{
		return m_error.Fail("Failed to flush memory card file");
	}

	if (sameFile && withEcc == m_impl->m_withEcc)
	{
		return true;
	}

	const std::filesystem::path dest(sameFile ? m_impl->m_filename : filename);

	if (withEcc == m_impl->m_withEcc)
	{
		m_impl->m_file.close();
		m_impl->m_pageCache.clear();
		m_impl->m_fatClusterCache.clear();

		std::filesystem::copy_file(m_impl->m_filename, dest,
			std::filesystem::copy_options::overwrite_existing, ec);
		if (ec)
		{
			m_impl->m_file.open(m_impl->m_filename, std::ios::in | std::ios::out | std::ios::binary);
			if (!m_impl->m_file)
			{
				m_impl->m_file.open(m_impl->m_filename, std::ios::in | std::ios::binary);
			}
			return m_error.Fail("Failed to copy memory card file: " + ec.message());
		}

		m_impl->m_file.open(m_impl->m_filename, std::ios::in | std::ios::out | std::ios::binary);
		if (!m_impl->m_file)
		{
			m_impl->m_file.open(m_impl->m_filename, std::ios::in | std::ios::binary);
		}
		if (!m_impl->m_file)
		{
			return m_error.Fail("Failed to reopen memory card after save");
		}
		return m_impl->readSuperblock();
	}

	const uint32_t pageCount = m_impl->m_clustersPerCard * m_impl->m_pagesPerCluster;
	const std::filesystem::path tmpPath = dest.string() + ".tmp";
	std::filesystem::remove(tmpPath, ec);

	std::ofstream out(tmpPath, std::ios::binary);
	if (!out)
	{
		return m_error.Fail("Failed to create file: " + tmpPath.string());
	}

	for (uint32_t i = 0; i < pageCount; ++i)
	{
		auto pageData = m_impl->readPage(i);
		if (pageData.empty())
		{
			out.close();
			std::filesystem::remove(tmpPath, ec);
			return false;
		}
		if (pageData.size() > m_impl->m_pageSize)
		{
			pageData.resize(m_impl->m_pageSize);
		}

		const char* MAGIC = "Sony PS2 Memory Card Format ";
		if (pageData.size() >= 28 && std::memcmp(pageData.data(), MAGIC, 28) == 0)
		{
			if (pageData.size() > 0x151)
			{
				if (withEcc)
				{
					pageData[0x151] |= 0x01;
				}
				else
				{
					pageData[0x151] &= ~0x01;
				}
			}
		}

		out.write(reinterpret_cast<const char*>(pageData.data()),
			static_cast<std::streamsize>(m_impl->m_pageSize));
		if (!out)
		{
			std::filesystem::remove(tmpPath, ec);
			return m_error.Fail("Failed to write memory card file");
		}

		if (withEcc && !m_impl->m_withEcc)
		{
			uint32_t targetSpareSize = divRoundUp(m_impl->m_pageSize, 128) * 4;
			std::vector<uint8_t> spareBytes(targetSpareSize, 0);
			auto ecc = eccCalculatePage(pageData, static_cast<int>(m_impl->m_pageSize));
			for (size_t j = 0; j < ecc.size() && j < targetSpareSize; ++j)
			{
				spareBytes[j] = ecc[j];
			}
			out.write(reinterpret_cast<const char*>(spareBytes.data()),
				static_cast<std::streamsize>(targetSpareSize));
			if (!out)
			{
				std::filesystem::remove(tmpPath, ec);
				return m_error.Fail("Failed to write memory card file");
			}
		}
	}

	out.close();

	if (!sameFile)
	{
		return renameReplace(dest, tmpPath, m_error);
	}

	m_impl->m_file.close();
	m_impl->m_pageCache.clear();
	m_impl->m_fatClusterCache.clear();
	if (!renameReplace(dest, tmpPath, m_error))
	{
		return false;
	}

	m_impl->m_withEcc = withEcc;
	if (withEcc)
	{
		m_impl->calculateDerived();
	}
	else
	{
		m_impl->m_spareSize = 0;
		m_impl->m_rawPageSize = m_impl->m_pageSize;
	}

	m_impl->m_file.open(m_impl->m_filename, std::ios::in | std::ios::out | std::ios::binary);
	if (!m_impl->m_file)
	{
		m_impl->m_file.open(m_impl->m_filename, std::ios::in | std::ios::binary);
	}
	if (!m_impl->m_file)
	{
		return m_error.Fail("Failed to reopen memory card after save");
	}
	return m_impl->readSuperblock();
}
