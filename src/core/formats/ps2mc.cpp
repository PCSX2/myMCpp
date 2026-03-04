// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "Logger.h"
#include "ps2mc.h"
#include "ps2mc_ecc.h"
#include "ps2mc_dir.h"
#include "ps2iconsys.h"
#include "ps2save.h"
#include "sjis.h"
#include "round.h"
#include <fstream>
#include <cstring>
#include <vector>
#include <array>
#include <map>
#include <memory>
#include <algorithm>
#include <ctime>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

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

class PS2MemoryCard::Impl
{
public:
	std::fstream file;
	std::string filename;

	uint32_t page_size = 512;
	uint32_t pages_per_cluster = 2;
	uint32_t cluster_size = 1024;
	uint32_t pages_per_erase_block = 16;
	uint32_t spare_size = 16; // ECC spare data size
	uint32_t raw_page_size = 528; // page_size + spare_size

	bool with_ecc = true;
	bool ignore_ecc = false;

	uint32_t clusters_per_card = 8192;
	uint32_t allocatable_cluster_offset = 0;
	uint32_t allocatable_cluster_end = 0;
	uint32_t rootdir_fat_cluster = 0;

	uint32_t entries_per_cluster = 256; // cluster_size / 4

	std::vector<uint32_t> fat;

	std::array<uint32_t, 32> indirect_fat_cluster_list = {};

	bool modified = false;
	std::map<uint32_t, std::vector<uint8_t>> page_cache;
	std::map<uint32_t, std::vector<uint8_t>> fat_cluster_cache;

	void calculate_derived();
	void read_superblock();
	std::vector<uint8_t> read_page(uint32_t page_num);
	void write_page(uint32_t page_num, const std::vector<uint8_t>& data);
	std::vector<uint8_t> read_cluster(uint32_t cluster_num);
	void write_cluster(uint32_t cluster_num, const std::vector<uint8_t>& data);
	std::vector<uint32_t> read_fat_cluster(uint32_t fat_cluster_num);
	void read_fat_from_card();
	void write_fat_to_card();
	uint32_t lookup_fat(uint32_t cluster_num);
	PS2McDirEntry find_entry(const std::string& path, uint32_t& parent_cluster);
	std::vector<PS2McDirEntry> read_dirents(uint32_t dir_cluster);
	void write_dirents(uint32_t dir_cluster, const std::vector<PS2McDirEntry>& entries);
	uint32_t allocate_cluster();
	std::vector<uint32_t> allocate_clusters(uint32_t count);
	void free_cluster_chain(uint32_t start_cluster);

	void init_card_parameters(int sizeInMB, bool disableEcc);
	void calculate_fat_layout(uint32_t& first_ifc, uint32_t& indirect_fat_clusters, uint32_t& fat_clusters);
	void create_empty_card_file(const std::string& filename, uint64_t totalBytes);
	void write_superblock(uint32_t first_ifc, uint32_t indirect_fat_clusters, uint32_t good_block1, uint32_t good_block2);
	void init_indirect_fat_clusters(uint32_t first_ifc, uint32_t indirect_fat_clusters, uint32_t epc);
	void init_root_directory();
};

void PS2MemoryCard::Impl::calculate_derived()
{
	spare_size = divRoundUp(page_size, 128) * 4;
	raw_page_size = page_size + spare_size;
	cluster_size = page_size * pages_per_cluster;
	entries_per_cluster = cluster_size / 4; // 4 bytes per FAT entry
}

std::vector<uint8_t> PS2MemoryCard::Impl::read_page(uint32_t page_num)
{
	if (!file.is_open())
	{
		throw PS2McIOError("Memory card not open");
	}

	if (page_cache.count(page_num))
	{
		return page_cache[page_num];
	}

	uint64_t offset = static_cast<uint64_t>(page_num) * raw_page_size;

	file.seekg(offset);
	if (!file)
	{
		throw PS2McIOError("Seek failed");
	}

	std::vector<uint8_t> page_data(page_size);
	file.read(reinterpret_cast<char*>(page_data.data()), page_size);

	if (file.gcount() != static_cast<std::streamsize>(page_size))
	{
		throw PS2McIOError("Failed to read complete page");
	}

	if (!ignore_ecc && spare_size > 0)
	{
		std::vector<uint8_t> spare(spare_size);
		file.read(reinterpret_cast<char*>(spare.data()), spare_size);

		if (file.gcount() != static_cast<std::streamsize>(spare_size))
		{
			throw PS2McIOError("Failed to read ECC spare data");
		}

		try
		{
			auto ecc_status = eccCheckPage(page_data, spare, page_size);
			(void)ecc_status;
		}
		catch (...)
		{
			// If ECC check fails, we might be dealing with a non-ECC card
			// Continue anyway
		}
	}

	page_cache[page_num] = page_data;
	return page_data;
}

void PS2MemoryCard::Impl::write_page(uint32_t page_num, const std::vector<uint8_t>& data)
{
	if (!file.is_open())
	{
		throw PS2McIOError("Memory card not open");
	}

	if (data.size() != page_size)
	{
		throw PS2McIOError("Invalid page data size");
	}

	uint64_t offset = static_cast<uint64_t>(page_num) * raw_page_size;
	file.seekp(offset);
	if (!file)
	{
		throw PS2McIOError("Seek failed");
	}

	file.write(reinterpret_cast<const char*>(data.data()), page_size);
	if (!file)
	{
		throw PS2McIOError("Failed to write page");
	}

	if (!ignore_ecc && spare_size > 0)
	{
		std::vector<uint8_t> spare(spare_size, 0);
		try
		{
			auto ecc_data = eccCalculatePage(data, page_size);
			for (size_t i = 0; i < ecc_data.size() && i < spare_size; ++i)
			{
				spare[i] = ecc_data[i];
			}
		}
		catch (...)
		{
			// If ECC calculation fails, just write zeros
		}
		file.write(reinterpret_cast<const char*>(spare.data()), spare_size);
	}

	modified = true;
	page_cache[page_num] = data;
}

std::vector<uint8_t> PS2MemoryCard::Impl::read_cluster(uint32_t cluster_num)
{
	std::vector<uint8_t> result;
	result.reserve(cluster_size);

	for (uint32_t i = 0; i < pages_per_cluster; ++i)
	{
		uint32_t page_num = cluster_num * pages_per_cluster + i;
		auto page_data = read_page(page_num);
		result.insert(result.end(), page_data.begin(), page_data.end());
	}

	return result;
}

void PS2MemoryCard::Impl::write_cluster(uint32_t cluster_num, const std::vector<uint8_t>& data)
{
	if (data.size() != cluster_size)
	{
		throw PS2McIOError("Invalid cluster data size");
	}

	for (uint32_t i = 0; i < pages_per_cluster; ++i)
	{
		uint32_t page_num = cluster_num * pages_per_cluster + i;
		std::vector<uint8_t> page_data(
			data.begin() + i * page_size,
			data.begin() + (i + 1) * page_size);
		write_page(page_num, page_data);
	}
}

std::vector<uint32_t> PS2MemoryCard::Impl::read_fat_cluster(uint32_t fat_cluster_num)
{
	if (fat_cluster_cache.count(fat_cluster_num))
	{
		auto cluster_data = fat_cluster_cache[fat_cluster_num];
		std::vector<uint32_t> result(entries_per_cluster);
		for (uint32_t i = 0; i < entries_per_cluster; ++i)
		{
			result[i] = readLE<uint32_t>(cluster_data, i * 4);
		}
		return result;
	}

	auto cluster_data = read_cluster(fat_cluster_num);
	std::vector<uint32_t> result(entries_per_cluster);
	for (uint32_t i = 0; i < entries_per_cluster; ++i)
	{
		result[i] = readLE<uint32_t>(cluster_data, i * 4);
	}

	fat_cluster_cache[fat_cluster_num] = cluster_data;
	return result;
}

void PS2MemoryCard::Impl::read_fat_from_card()
{
	fat.clear();
	fat.resize(allocatable_cluster_end, PS2MC_FAT_CHAIN_END);

	uint32_t fat_entry = 0;

	for (uint32_t dbl_offset = 0; dbl_offset < 32 && fat_entry < fat.size(); ++dbl_offset)
	{
		uint32_t indirect_cluster = indirect_fat_cluster_list[dbl_offset];

		if (indirect_cluster == 0 || indirect_cluster == 0xFFFFFFFF)
		{
			continue;
		}

		try
		{
			auto indirect_data = read_cluster(indirect_cluster);

			for (uint32_t indirect_offset = 0; indirect_offset < entries_per_cluster && fat_entry < fat.size(); ++indirect_offset)
			{
				uint32_t fat_cluster_ptr = readLE<uint32_t>(indirect_data, indirect_offset * 4);

				if (fat_cluster_ptr == 0 || fat_cluster_ptr == 0xFFFFFFFF)
				{
					continue;
				}

				auto fat_cluster = read_fat_cluster(fat_cluster_ptr);

				for (uint32_t j = 0; j < fat_cluster.size(); ++j)
				{
					if (fat_entry >= fat.size())
					{
						break;
					}
					fat[fat_entry] = fat_cluster[j];
					fat_entry++;
				}
			}
		}
		catch (...)
		{
			// Skip bad clusters and continue
			continue;
		}
	}
}

void PS2MemoryCard::Impl::read_superblock()
{
	auto sb_page = read_page(0);

	const char* MAGIC = "Sony PS2 Memory Card Format ";
	if (sb_page.size() < 28 || std::memcmp(sb_page.data(), MAGIC, 28) != 0)
	{
		throw PS2McCorrupt("Invalid memory card magic");
	}

	page_size = readLE<uint16_t>(sb_page, 40);
	pages_per_cluster = readLE<uint16_t>(sb_page, 42);
	pages_per_erase_block = readLE<uint16_t>(sb_page, 44);

	if (page_size == 0 || pages_per_cluster == 0)
	{
		throw PS2McCorrupt("Invalid page/cluster size in superblock");
	}

	calculate_derived();

	clusters_per_card = readLE<uint32_t>(sb_page, 48); // offset 48-51
	allocatable_cluster_offset = readLE<uint32_t>(sb_page, 52); // offset 52-55
	allocatable_cluster_end = readLE<uint32_t>(sb_page, 56); // offset 56-59
	rootdir_fat_cluster = readLE<uint32_t>(sb_page, 60); // offset 60-63

	for (int i = 0; i < 32; ++i)
	{
		indirect_fat_cluster_list[i] = readLE<uint32_t>(sb_page, 80 + i * 4);
	}

	if (allocatable_cluster_end == 0 || allocatable_cluster_end > 1000000)
	{
		allocatable_cluster_end = clusters_per_card;
	}

	{
		uint32_t total_pages = clusters_per_card * pages_per_cluster;
		uint64_t expected_with_ecc = static_cast<uint64_t>(total_pages) * (page_size + spare_size);
		uint64_t expected_no_ecc = static_cast<uint64_t>(total_pages) * page_size;

		file.seekg(0, std::ios::end);
		uint64_t file_size = static_cast<uint64_t>(file.tellg());
		file.seekg(0, std::ios::beg);

		if (file_size <= expected_no_ecc || file_size < expected_with_ecc)
		{
			spare_size = 0;
			raw_page_size = page_size;
			ignore_ecc = true;
			with_ecc = false;
			page_cache.clear();
		}
	}

	read_fat_from_card();
}

uint32_t PS2MemoryCard::Impl::lookup_fat(uint32_t cluster_num)
{
	if (cluster_num >= fat.size())
	{
		throw PS2McIOError("FAT index out of range");
	}
	return fat[cluster_num];
}

PS2McDirEntry PS2MemoryCard::Impl::find_entry(const std::string& path, uint32_t& parent_cluster)
{
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

	uint32_t current_cluster = rootdir_fat_cluster;
	parent_cluster = current_cluster;

	for (size_t i = 0; i < parts.size(); ++i)
	{
		const auto& part = parts[i];
		auto entries = read_dirents(current_cluster);

		PS2McDirEntry found_entry;
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
			throw PS2McPathNotFound(path);
		}

		if (i < parts.size() - 1)
		{
			if (!(found_entry.mode & DF_DIR))
			{
				throw PS2McIOError("Path component is not a directory");
			}
		}

		parent_cluster = current_cluster;
		current_cluster = found_entry.cluster;
	}

	if (parts.empty())
	{
		PS2McDirEntry root;
		root.mode = DF_DIR | DF_EXISTS;
		root.name = "/";
		root.cluster = rootdir_fat_cluster;
		return root;
	}

	auto entries = read_dirents(parent_cluster);
	for (const auto& e : entries)
	{
		if (e.name == parts.back())
		{
			return e;
		}
	}

	throw PS2McPathNotFound(path);
}

std::vector<PS2McDirEntry> PS2MemoryCard::Impl::read_dirents(uint32_t dir_cluster)
{
	std::vector<PS2McDirEntry> entries;

	uint32_t current_cluster = dir_cluster;
	int iteration = 0;

	while (current_cluster != PS2MC_FAT_CHAIN_END && current_cluster < fat.size())
	{
		iteration++;
		if (iteration > 1000)
		{ // Safety limit
			break;
		}
		uint32_t disk_cluster = current_cluster + allocatable_cluster_offset;
		auto cluster_data = read_cluster(disk_cluster);

		uint32_t entries_per_cluster_block = cluster_size / PS2MC_DIRENT_LENGTH;

		for (uint32_t i = 0; i < entries_per_cluster_block; ++i)
		{
			uint32_t offset = i * PS2MC_DIRENT_LENGTH;
			if (offset + PS2MC_DIRENT_LENGTH > cluster_data.size())
				break;

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

		if (current_cluster >= fat.size())
		{
			break;
		}

		uint32_t next = fat[current_cluster];

		if ((next & PS2MC_FAT_CLUSTER_MASK) == PS2MC_FAT_CHAIN_END_UNALLOC)
		{
			break;
		}

		next = next & PS2MC_FAT_CLUSTER_MASK;

		if (next >= fat.size() || next == PS2MC_FAT_CHAIN_END_UNALLOC)
		{
			break;
		}

		current_cluster = next;
	}

	return entries;
}

void PS2MemoryCard::Impl::write_dirents(uint32_t dir_cluster, const std::vector<PS2McDirEntry>& entries)
{
	uint32_t current_cluster = dir_cluster;
	uint32_t entry_idx = 0;
	int iteration = 0;

	while (entry_idx < entries.size())
	{
		iteration++;
		if (iteration > 1000)
		{
			break;
		}

		std::vector<uint8_t> cluster_data(cluster_size, 0);
		uint32_t entries_per_cluster_block = cluster_size / PS2MC_DIRENT_LENGTH;

		for (uint32_t i = 0; i < entries_per_cluster_block && entry_idx < entries.size(); ++i, ++entry_idx)
		{
			auto packed = packDirEntry(entries[entry_idx]);
			uint32_t offset = i * PS2MC_DIRENT_LENGTH;
			std::copy(packed.begin(), packed.end(), cluster_data.begin() + offset);
		}

		uint32_t disk_cluster = current_cluster + allocatable_cluster_offset;
		write_cluster(disk_cluster, cluster_data);

		if (entry_idx < entries.size())
		{
			uint32_t next = fat[current_cluster] & PS2MC_FAT_CLUSTER_MASK;
			
			if (next >= fat.size() || next == PS2MC_FAT_CHAIN_END_UNALLOC || next == PS2MC_FAT_CHAIN_END)
			{
				uint32_t new_cluster = allocate_cluster();
				fat[current_cluster] = (fat[current_cluster] & ~PS2MC_FAT_CLUSTER_MASK) | new_cluster;
				fat[new_cluster] = PS2MC_FAT_CHAIN_END;
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
}

uint32_t PS2MemoryCard::Impl::allocate_cluster()
{
	for (uint32_t i = 0; i < fat.size(); ++i)
	{
		if ((fat[i] & PS2MC_FAT_ALLOCATED_BIT) == 0 && fat[i] == PS2MC_FAT_CHAIN_END_UNALLOC)
		{
			fat[i] = PS2MC_FAT_CHAIN_END; // Mark as end of chain
			modified = true;
			return i;
		}
	}

	throw PS2McIOError("No free clusters available on memory card");
}

std::vector<uint32_t> PS2MemoryCard::Impl::allocate_clusters(uint32_t count)
{
	std::vector<uint32_t> clusters;
	clusters.reserve(count);

	for (uint32_t i = 0; i < count; ++i)
	{
		uint32_t cluster = allocate_cluster();
		clusters.push_back(cluster);

		if (i > 0)
		{
			fat[clusters[i - 1]] = cluster;
		}
	}

	if (!clusters.empty())
	{
		fat[clusters.back()] = PS2MC_FAT_CHAIN_END;
	}

	return clusters;
}

void PS2MemoryCard::Impl::free_cluster_chain(uint32_t start_cluster)
{
	uint32_t current = start_cluster;

	while (current != PS2MC_FAT_CHAIN_END && current < fat.size())
	{
		uint32_t next = fat[current] & PS2MC_FAT_CLUSTER_MASK;
		fat[current] = PS2MC_FAT_CHAIN_END_UNALLOC;
		current = next;
	}

	modified = true;
}

void PS2MemoryCard::Impl::write_fat_to_card()
{
	uint32_t fat_entry_idx = 0;

	for (int i = 0; i < 32; ++i)
	{
		uint32_t indirect_cluster = indirect_fat_cluster_list[i];
		if (indirect_cluster == 0 || indirect_cluster == 0xFFFFFFFF)
			continue;

		auto indirect_data = read_cluster(indirect_cluster);
		for (uint32_t j = 0; j < entries_per_cluster; ++j)
		{
			uint32_t fat_cluster_phys = readLE<uint32_t>(indirect_data, j * 4);

			if (fat_cluster_phys == 0 || fat_cluster_phys == 0xFFFFFFFF)
				continue;

			if (fat_entry_idx >= fat.size())
				break;

			std::vector<uint8_t> fat_data(cluster_size, 0);

			for (uint32_t k = 0; k < entries_per_cluster && fat_entry_idx < fat.size(); ++k)
			{
				uint32_t entry = fat[fat_entry_idx++];
				writeLE(fat_data, k * 4, entry);
			}
			
			write_cluster(fat_cluster_phys, fat_data);

			if (fat_entry_idx >= fat.size())
				break;
		}
		
		if (fat_entry_idx >= fat.size())
			break;
	}
}

PS2MemoryCard::PS2MemoryCard()
	: pImpl(std::make_unique<Impl>())
{
}
PS2MemoryCard::~PS2MemoryCard()
{
	try
	{
		close();
	}
	catch (...)
	{
		// ignore
	}
}

void PS2MemoryCard::open(const std::string& path)
{
	pImpl->filename = path;
	pImpl->file.open(path, std::ios::in | std::ios::out | std::ios::binary);

	if (!pImpl->file)
	{
		pImpl->file.open(path, std::ios::in | std::ios::binary);
		if (!pImpl->file)
		{
			throw PS2McIOError("Failed to open memory card: " + path);
		}
	}

	pImpl->read_superblock();
}

void PS2MemoryCard::close()
{
	if (pImpl->file.is_open())
	{
		pImpl->file.close();
	}
}

void PS2MemoryCard::Impl::init_card_parameters(int sizeInMB, bool disableEcc)
{
	page_size = 512;
	pages_per_cluster = 2;
	pages_per_erase_block = 16;
	clusters_per_card = (sizeInMB * 1024 * 1024) / 1024;
	with_ecc = !disableEcc;
	ignore_ecc = disableEcc;

	if (disableEcc)
	{
		spare_size = 0;
		raw_page_size = page_size;
	}
	else
	{
		calculate_derived();
	}
}

void PS2MemoryCard::Impl::calculate_fat_layout(uint32_t& first_ifc, uint32_t& indirect_fat_clusters_out, uint32_t& fat_clusters_out)
{
	first_ifc = 8;
	uint32_t epc = cluster_size / 4;

	uint32_t allocatable_clusters_est = clusters_per_card - (first_ifc + 2);
	uint32_t fat_clusters = (allocatable_clusters_est + epc - 1) / epc;
	uint32_t indirect_fat_clusters = (fat_clusters + epc - 1) / epc;
	if (indirect_fat_clusters > 32)
		indirect_fat_clusters = 32;

	fat_clusters = indirect_fat_clusters * epc;

	allocatable_cluster_offset = first_ifc + indirect_fat_clusters + fat_clusters;

	uint32_t pages_per_card = clusters_per_card * pages_per_cluster;
	uint32_t erase_blocks_per_card = pages_per_card / pages_per_erase_block;
	uint32_t good_block2 = erase_blocks_per_card - 2;
	uint32_t clusters_per_erase_block = pages_per_erase_block / pages_per_cluster;

	allocatable_cluster_end = (good_block2 * clusters_per_erase_block) - allocatable_cluster_offset;
	rootdir_fat_cluster = 0;

	indirect_fat_clusters_out = indirect_fat_clusters;
	fat_clusters_out = fat_clusters;
}

void PS2MemoryCard::Impl::create_empty_card_file(const std::string& fname, uint64_t totalBytes)
{
	std::ofstream ofs(fname, std::ios::binary | std::ios::out);
	if (!ofs)
	{
		throw PS2McIOError("Could not create file: " + fname);
	}

	std::vector<char> chunk(65536, 0);
	uint64_t written = 0;
	while (written < totalBytes)
	{
		uint64_t toWrite = (std::min)((uint64_t)chunk.size(), totalBytes - written);
		ofs.write(chunk.data(), toWrite);
		written += toWrite;
	}
}

void PS2MemoryCard::Impl::write_superblock(uint32_t first_ifc, uint32_t indirect_fat_clusters, uint32_t good_block1, uint32_t good_block2)
{
	std::vector<uint8_t> sb(page_size, 0);
	std::memcpy(sb.data(), "Sony PS2 Memory Card Format ", 28);
	std::memcpy(sb.data() + 28, "1.2.0.0", 8);

	writeLE<uint16_t>(sb, 40, static_cast<uint16_t>(page_size));
	writeLE<uint16_t>(sb, 42, static_cast<uint16_t>(pages_per_cluster));
	writeLE<uint16_t>(sb, 44, static_cast<uint16_t>(pages_per_erase_block));
	writeLE<uint16_t>(sb, 46, 0xFF00);
	writeLE<uint32_t>(sb, 48, clusters_per_card);
	writeLE<uint32_t>(sb, 52, allocatable_cluster_offset);
	writeLE<uint32_t>(sb, 56, allocatable_cluster_end);
	writeLE<uint32_t>(sb, 60, rootdir_fat_cluster);
	writeLE<uint32_t>(sb, 64, good_block1);
	writeLE<uint32_t>(sb, 68, good_block2);

	for (uint32_t i = 0; i < indirect_fat_clusters; ++i)
	{
		uint32_t ifc_phys = first_ifc + i;
		indirect_fat_cluster_list[i] = ifc_phys;
		writeLE<uint32_t>(sb, 80 + i * 4, ifc_phys);
	}

	for (int i = 0; i < 32; ++i)
	{
		writeLE<uint32_t>(sb, 208 + i * 4, 0xFFFFFFFF);
	}

	sb[336] = 2;
	sb[337] = with_ecc ? 0x52 : 0x50;

	write_page(0, sb);
}

void PS2MemoryCard::Impl::init_indirect_fat_clusters(uint32_t first_ifc, uint32_t indirect_fat_clusters, uint32_t epc)
{
	uint32_t current_fat_cluster_phys = first_ifc + indirect_fat_clusters;

	for (uint32_t i = 0; i < indirect_fat_clusters; ++i)
	{
		uint32_t ifc_phys = first_ifc + i;
		std::vector<uint8_t> ifc_data(cluster_size, 0);

		for (uint32_t j = 0; j < epc; ++j)
		{
			if (current_fat_cluster_phys < allocatable_cluster_offset)
			{
				writeLE<uint32_t>(ifc_data, j * 4, current_fat_cluster_phys++);
			}
			else
			{
				writeLE<uint32_t>(ifc_data, j * 4, 0xFFFFFFFF);
			}
		}
		write_cluster(ifc_phys, ifc_data);
	}
}

void PS2MemoryCard::Impl::init_root_directory()
{
	fat.clear();
	fat.resize(allocatable_cluster_end, PS2MC_FAT_CHAIN_END_UNALLOC);
	fat[0] = PS2MC_FAT_CHAIN_END;

	write_fat_to_card();

	std::vector<PS2McDirEntry> rootEntries;

	PS2McDirEntry dot;
	dot.mode = DF_DIR | DF_EXISTS | DF_RWX | DF_0400;
	dot.name = ".";
	dot.cluster = 0;
	dot.length = 2;
	dot.created = timeToTod(time(nullptr));
	dot.modified = dot.created;
	rootEntries.push_back(dot);

	PS2McDirEntry dotdot;
	dotdot.mode = DF_DIR | DF_EXISTS | DF_WRITE | DF_EXECUTE | DF_0400 | DF_HIDDEN;
	dotdot.name = "..";
	dotdot.cluster = 0;
	dotdot.length = 0;
	dotdot.created = dot.created;
	dotdot.modified = dot.modified;
	rootEntries.push_back(dotdot);

	write_dirents(0, rootEntries);
}

void PS2MemoryCard::create(const std::string& filename, int sizeInMB, bool disableEcc)
{
	close();

	pImpl->filename = filename;
	pImpl->init_card_parameters(sizeInMB, disableEcc);

	uint32_t first_ifc, indirect_fat_clusters, fat_clusters;
	pImpl->calculate_fat_layout(first_ifc, indirect_fat_clusters, fat_clusters);

	uint32_t pages_per_card = pImpl->clusters_per_card * pImpl->pages_per_cluster;
	uint32_t erase_blocks_per_card = pages_per_card / pImpl->pages_per_erase_block;
	uint32_t good_block1 = erase_blocks_per_card - 1;
	uint32_t good_block2 = erase_blocks_per_card - 2;

	uint64_t totalBytes = static_cast<uint64_t>(pImpl->clusters_per_card) *
						  pImpl->pages_per_cluster * pImpl->raw_page_size;

	pImpl->create_empty_card_file(filename, totalBytes);

	pImpl->file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
	if (!pImpl->file)
	{
		throw PS2McIOError("Failed to open created card");
	}

	pImpl->write_superblock(first_ifc, indirect_fat_clusters, good_block1, good_block2);

	uint32_t epc = pImpl->cluster_size / 4;
	pImpl->init_indirect_fat_clusters(first_ifc, indirect_fat_clusters, epc);

	pImpl->init_root_directory();
}

std::vector<PS2McDirEntry> PS2MemoryCard::listDir(const std::string& path)
{
	std::string check_path = path.empty() ? "/" : path;

	if (check_path == "/")
	{
		return pImpl->read_dirents(pImpl->rootdir_fat_cluster);
	}

	uint32_t parent_cluster = 0;
	auto entry = pImpl->find_entry(check_path, parent_cluster);

	if (!(entry.mode & DF_DIR))
	{
		throw PS2McIOError("Not a directory: " + path);
	}

	return pImpl->read_dirents(entry.cluster);
}

PS2McDirEntry PS2MemoryCard::getEntry(const std::string& path)
{
	if (path == "/")
	{
		PS2McDirEntry root;
		root.mode = DF_DIR | DF_EXISTS;
		root.name = "/";
		root.cluster = pImpl->rootdir_fat_cluster;
		return root;
	}

	uint32_t parent_cluster = 0;
	return pImpl->find_entry(path, parent_cluster);
}

std::vector<uint8_t> PS2MemoryCard::readFile(const std::string& path)
{
	uint32_t parent_cluster = 0;
	auto entry = pImpl->find_entry(path, parent_cluster);

	if (entry.mode & DF_DIR)
	{
		throw PS2McIOError("Cannot read directory as file");
	}

	std::vector<uint8_t> result;
	result.reserve(entry.length);

	// Follow the FAT chain from entry.cluster
	uint32_t current_cluster = entry.cluster;
	uint32_t bytes_read = 0;

	while (bytes_read < entry.length && current_cluster != PS2MC_FAT_CHAIN_END && current_cluster < pImpl->fat.size())
	{
		uint32_t disk_cluster = current_cluster + pImpl->allocatable_cluster_offset;
		auto cluster_data = pImpl->read_cluster(disk_cluster);
		uint32_t remaining = entry.length - bytes_read;
		uint32_t to_read = remaining < pImpl->cluster_size ? remaining : pImpl->cluster_size;

		result.insert(result.end(),
			cluster_data.begin(),
			cluster_data.begin() + to_read);
		bytes_read += to_read;

		uint32_t next = pImpl->fat[current_cluster] & PS2MC_FAT_CLUSTER_MASK;
		if (next == PS2MC_FAT_CHAIN_END_UNALLOC)
		{
			break;
		}
		current_cluster = next;
	}

	return result;
}

void PS2MemoryCard::exportFile(const std::string& path, const std::string& dest_path)
{
	auto data = readFile(path);
	std::ofstream outfile(dest_path, std::ios::binary);
	if (!outfile)
	{
		throw PS2McIOError("Failed to create export file: " + dest_path);
	}
	outfile.write(reinterpret_cast<const char*>(data.data()), data.size());
}

std::vector<uint8_t> PS2MemoryCard::getIconData(const std::string& savePath)
{
	Logger::debug("getIconData: Trying to load icon for path: {}", savePath);

	try
	{
		std::string iconSysPath = savePath + "/icon.sys";
		Logger::debug("getIconData: Trying to read icon.sys at: {}", iconSysPath);
		auto iconSysData = readFile(iconSysPath);
		if (!iconSysData.empty())
		{
			Logger::debug("getIconData: icon.sys loaded, size: {}", iconSysData.size());
			PS2IconSys iconSys;
			iconSys.load(iconSysData);

			std::string iconFile = iconSys.getIconFileNormal();
			Logger::debug("getIconData: icon.sys specifies icon file: {}", iconFile);
			if (!iconFile.empty())
			{
				try
				{
					std::string iconPath = savePath + "/" + iconFile;
					Logger::debug("getIconData: Trying to read icon at: {}", iconPath);
					auto data = readFile(iconPath);
					if (!data.empty())
					{
						Logger::debug("getIconData: Successfully loaded icon, size: {}", data.size());
						return data;
					}
				}
				catch (const std::exception& e)
				{
					Logger::debug("getIconData: Exception reading icon file: {}", e.what());
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		Logger::debug("getIconData: Exception reading icon.sys: {}", e.what());
	}

	std::vector<std::string> iconPaths = {
		savePath + "/icon0.icn",
		savePath + "/icon1.icn",
		savePath + "/icon.icn"};

	for (const auto& iconPath : iconPaths)
	{
		try
		{
			auto data = readFile(iconPath);
			if (!data.empty())
			{
				return data;
			}
		}
		catch (...)
		{
			// Try next icon file
			continue;
		}
	}

	// Return empty vector if no icon found
	return std::vector<uint8_t>();
}

PS2IconSys* PS2MemoryCard::getIconSys(const std::string& savePath)
{
	try
	{
		std::string iconSysPath = savePath + "/icon.sys";
		auto iconSysData = readFile(iconSysPath);

		if (iconSysData.empty())
		{
			return nullptr;
		}

		PS2IconSys* iconSys = new PS2IconSys();
		iconSys->load(iconSysData);

		return iconSys;
	}
	catch (const std::exception& e)
	{
		Logger::debug("getIconSys: Exception: {}", e.what());
		return nullptr;
	}
}

std::string PS2MemoryCard::getSaveTitle(const std::string& savePath)
{
	try
	{
		auto iconData = readFile(savePath + "/icon.sys");
		if (iconData.empty())
		{
			return "";
		}

		PS2IconSys iconSys;
		iconSys.load(iconData);
		return iconSys.getTitle();
	}
	catch (...)
	{
		return "";
	}
}

std::string PS2MemoryCard::getSaveSubtitle(const std::string& savePath)
{
	try
	{
		auto iconData = readFile(savePath + "/icon.sys");
		if (iconData.empty())
		{
			return "";
		}

		PS2IconSys iconSys;
		iconSys.load(iconData);
		return iconSys.getSubtitle();
	}
	catch (...)
	{
		return "";
	}
}

uint32_t PS2MemoryCard::getSaveSize(const std::string& savePath)
{
	try
	{
		auto entries = listDir(savePath);

		uint32_t totalSize = roundUp(static_cast<uint32_t>(entries.size()) * PS2MC_DIRENT_LENGTH, pImpl->cluster_size);

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
				totalSize += roundUp(entry.length, pImpl->cluster_size);
			}
		}

		return totalSize;
	}
	catch (...)
	{
		return 0;
	}
}

uint32_t PS2MemoryCard::getFreeSpace()
{
	uint32_t free_clusters = 0;
	for (uint32_t entry : pImpl->fat)
	{
		if ((entry & PS2MC_FAT_ALLOCATED_BIT) == 0 && entry == PS2MC_FAT_CHAIN_END_UNALLOC)
		{
			free_clusters++;
		}
	}
	return free_clusters * pImpl->cluster_size;
}

void PS2MemoryCard::makeDir(const std::string& path)
{
	if (path == "/" || path.empty())
	{
		throw PS2McIOError("Invalid directory path");
	}

	try
	{
		try
		{
			auto entry = pImpl->find_entry(path, pImpl->rootdir_fat_cluster);
			if (entry.mode & DF_EXISTS)
			{
				return;
			}
		}
		catch (const PS2McIOError&)
		{
		}

		size_t last_slash = path.rfind('/');
		std::string parent_path = (last_slash == 0) ? "/" : path.substr(0, last_slash);
		std::string dir_name = path.substr(last_slash + 1);

		uint32_t parent_cluster = 0;
		if (parent_path != "/")
		{
			auto parent_entry = pImpl->find_entry(parent_path, parent_cluster);
			parent_cluster = parent_entry.cluster;
		}
		else
		{
			parent_cluster = pImpl->rootdir_fat_cluster;
		}

		uint32_t dir_cluster = pImpl->allocate_cluster();

		PS2McDirEntry new_dir_entry = {};
		new_dir_entry.mode = DF_DIR | DF_EXISTS | DF_RWX;
		new_dir_entry.name = dir_name;
		new_dir_entry.cluster = dir_cluster;
		new_dir_entry.length = 0;
		new_dir_entry.created = timeToTod(time(nullptr));
		new_dir_entry.modified = new_dir_entry.created;

		std::vector<PS2McDirEntry> new_dir_entries;

		PS2McDirEntry dot_entry = {};
		dot_entry.mode = DF_DIR | DF_EXISTS | DF_RWX;
		dot_entry.name = ".";
		dot_entry.cluster = dir_cluster;
		dot_entry.length = 0;
		dot_entry.created = new_dir_entry.created;
		dot_entry.modified = new_dir_entry.modified;
		dot_entry.dirEntry = 0;
		new_dir_entries.push_back(dot_entry);

		PS2McDirEntry dotdot_entry = {};
		dotdot_entry.mode = DF_DIR | DF_EXISTS | DF_RWX;
		dotdot_entry.name = "..";
		dotdot_entry.cluster = parent_cluster;
		dotdot_entry.length = 0;
		dotdot_entry.created = new_dir_entry.created;
		dotdot_entry.modified = new_dir_entry.modified;
		dotdot_entry.dirEntry = 0;
		new_dir_entries.push_back(dotdot_entry);

		pImpl->write_dirents(dir_cluster, new_dir_entries);
		auto parent_entries = pImpl->read_dirents(parent_cluster);
		parent_entries.push_back(new_dir_entry);

		pImpl->write_dirents(parent_cluster, parent_entries);

		pImpl->write_fat_to_card();

		pImpl->modified = true;
	}
	catch (const PS2McIOError&)
	{
		throw;
	}
	catch (const std::exception& e)
	{
		throw PS2McIOError(std::string("Failed to create directory: ") + e.what());
	}
}

void PS2MemoryCard::writeFile(const std::string& path, const std::vector<uint8_t>& data)
{
	if (path == "/" || path.empty())
	{
		throw PS2McIOError("Invalid file path");
	}

	try
	{
		try
		{
			auto entry = pImpl->find_entry(path, pImpl->rootdir_fat_cluster);
			if (entry.mode & DF_EXISTS)
			{
				throw PS2McIOError("File already exists: " + path);
			}
		}
		catch (const PS2McPathNotFound&)
		{
		}

		size_t last_slash = path.rfind('/');
		std::string parent_path = (last_slash == 0) ? "/" : path.substr(0, last_slash);
		std::string file_name = path.substr(last_slash + 1);

		uint32_t parent_cluster = 0;
		if (parent_path != "/")
		{
			auto parent_entry = pImpl->find_entry(parent_path, parent_cluster);
			parent_cluster = parent_entry.cluster;
		}
		else
		{
			parent_cluster = pImpl->rootdir_fat_cluster;
		}

		// Calculate clusters needed
		uint32_t clusters_needed = (static_cast<uint32_t>(data.size()) + pImpl->cluster_size - 1) / pImpl->cluster_size;
		auto file_clusters = pImpl->allocate_clusters(clusters_needed);

		uint32_t offset = 0;
		for (uint32_t cluster : file_clusters)
		{
			uint32_t remaining = static_cast<uint32_t>(data.size()) - offset;
			uint32_t to_write = (pImpl->cluster_size < remaining) ? pImpl->cluster_size : remaining;
			std::vector<uint8_t> cluster_data(pImpl->cluster_size, 0);

			if (to_write > 0)
			{
				std::copy(data.begin() + offset, data.begin() + offset + to_write, cluster_data.begin());
			}

			uint32_t disk_cluster = cluster + pImpl->allocatable_cluster_offset;
			pImpl->write_cluster(disk_cluster, cluster_data);

			offset += to_write;
		}

		PS2McDirEntry file_entry = {};
		file_entry.mode = DF_FILE | DF_EXISTS | DF_RWX;
		file_entry.name = file_name;
		file_entry.cluster = file_clusters[0];
		file_entry.length = static_cast<uint32_t>(data.size());
		file_entry.created = timeToTod(time(nullptr));
		file_entry.modified = file_entry.created;

		auto parent_entries = pImpl->read_dirents(parent_cluster);
		parent_entries.push_back(file_entry);
		pImpl->write_dirents(parent_cluster, parent_entries);

		pImpl->write_fat_to_card();

		pImpl->modified = true;
	}
	catch (const PS2McIOError&)
	{
		throw;
	}
	catch (const std::exception& e)
	{
		throw PS2McIOError(std::string("Failed to write file: ") + e.what());
	}
}

void PS2MemoryCard::remove(const std::string& path)
{
	if (path == "/")
	{
		throw PS2McIOError("Cannot delete root directory");
	}

	try
	{
		uint32_t parent_cluster = 0;
		auto entry = pImpl->find_entry(path, parent_cluster);

		if (!(entry.mode & DF_EXISTS))
		{
			throw PS2McIOError("File not found: " + path);
		}

		entry.mode &= ~DF_EXISTS;

		if (entry.mode & DF_DIR)
		{
			try
			{
				auto contents = listDir(path);
				for (const auto& sub : contents)
				{
					if (sub.name == "." || sub.name == "..")
						continue;
					if (sub.mode & DF_EXISTS)
					{
						remove(path + "/" + sub.name);
					}
				}
			}
			catch (...)
			{
			}

			uint32_t cluster = entry.cluster;
			while (cluster != PS2MC_FAT_CHAIN_END && cluster < pImpl->fat.size())
			{
				uint32_t next = pImpl->fat[cluster] & PS2MC_FAT_CLUSTER_MASK;
				pImpl->fat[cluster] = PS2MC_FAT_CHAIN_END_UNALLOC;
				cluster = next;
			}
		}
		else
		{
			uint32_t cluster = entry.cluster;
			while (cluster != PS2MC_FAT_CHAIN_END && cluster < pImpl->fat.size())
			{
				uint32_t next = pImpl->fat[cluster] & PS2MC_FAT_CLUSTER_MASK;
				pImpl->fat[cluster] = PS2MC_FAT_CHAIN_END_UNALLOC;
				cluster = next;
			}
		}

		auto parent_entries = pImpl->read_dirents(parent_cluster);
		for (auto& e : parent_entries)
		{
			if (e.name == entry.name)
			{
				e.mode = entry.mode; // Update with deleted flag
				break;
			}
		}

		pImpl->write_dirents(parent_cluster, parent_entries);

		pImpl->modified = true;
	}
	catch (const PS2McIOError&)
	{
		throw;
	}
	catch (const std::exception& e)
	{
		throw PS2McIOError(std::string("Failed to delete: ") + e.what());
	}
}
bool PS2MemoryCard::importSaveFile(PS2SaveFile& save, bool ignoreExisting, const std::string& targetDir)
{
	if (!pImpl || !pImpl->file.is_open())
	{
		throw PS2McError("No memory card open");
	}

	try
	{
		const auto& entries = save.getEntries();
		if (entries.empty())
		{
			throw PS2McError("Save file contains no entries");
		}

		const auto& dirEntry = entries[0].dirEntry;
		std::string saveDirName = targetDir.empty() ? dirEntry.name : targetDir;

		if (!saveDirName.empty() && saveDirName[0] == '/')
		{
			saveDirName = saveDirName.substr(1);
		}

		std::string savePath = "/" + saveDirName;

		try
		{
			uint32_t dummy;
			pImpl->find_entry(savePath, dummy);

			if (!ignoreExisting)
			{
				return false; // Save already exists, not imported
			}

			remove(savePath);
		}
		catch (const PS2McPathNotFound&)
		{
		}

		makeDir(savePath);

		for (size_t i = 1; i < entries.size(); ++i)
		{
			const auto& entry = entries[i];
			std::string filePath = savePath + "/" + entry.dirEntry.name;

			writeFile(filePath, entry.data);

			uint32_t parent_cluster = 0;
			auto currentEntry = pImpl->find_entry(filePath, parent_cluster);
			auto parentEntries = pImpl->read_dirents(parent_cluster);
			bool updated = false;
			for (auto& pe : parentEntries)
			{
				if (pe.name == currentEntry.name)
				{
					pe.created = entry.dirEntry.created;
					pe.modified = entry.dirEntry.modified;
					pe.mode = (entry.dirEntry.mode & ~DF_DIR) | DF_EXISTS | DF_FILE;
					updated = true;
					break;
				}
			}
			if (updated)
			{
				pImpl->write_dirents(parent_cluster, parentEntries);
			}
		}

		pImpl->write_fat_to_card();
		pImpl->file.flush();

		return true;
	}
	catch (const std::exception& e)
	{
		throw PS2McError(std::string("Import failed: ") + e.what());
	}
}

void PS2MemoryCard::exportSaveFile(const std::string& savePath, PS2SaveFile& save)
{
	if (!pImpl || !pImpl->file.is_open())
	{
		throw PS2McError("No memory card open");
	}

	try
	{
		// Read the save directory entry
		uint32_t parent_cluster;
		PS2McDirEntry dirEntry = pImpl->find_entry(savePath, parent_cluster);

		if (!(dirEntry.mode & DF_DIR))
		{
			throw PS2McError("Path is not a directory");
		}

		// Create PS2SaveEntry for the directory itself
		PS2SaveEntry dirSaveEntry;
		dirSaveEntry.dirEntry = dirEntry;

		auto& entries = save.getEntries();
		entries.clear();
		entries.push_back(dirSaveEntry);

		// Read all files in the directory
		auto dirContents = pImpl->read_dirents(dirEntry.cluster);

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

			// Only export files, not subdirectories
			if (entry.mode & DF_FILE)
			{
				PS2SaveEntry fileEntry;
				fileEntry.dirEntry = entry;

				std::string filePath = savePath + "/" + entry.name;
				fileEntry.data = readFile(filePath);

				entries.push_back(fileEntry);
			}
		}

		// Set the save title from icon.sys if available
		try
		{
			std::string title = getSaveTitle(savePath);
			if (!title.empty())
			{
				save.setTitle(title);
			}
		}
		catch (...)
		{
			// Ignore if we can't get the title
		}
	}
	catch (const std::exception& e)
	{
		throw PS2McError(std::string("Export failed: ") + e.what());
	}
}

uint16_t PS2MemoryCard::getMode(const std::string& path)
{
	if (!pImpl->file.is_open())
	{
		throw PS2McError("Memory card not open");
	}

	uint32_t parent_cluster = 0;
	auto entry = pImpl->find_entry(path, parent_cluster);
	return entry.mode;
}

void PS2MemoryCard::setMode(const std::string& path, uint16_t mode)
{
	if (!pImpl->file.is_open())
	{
		throw PS2McError("Memory card not open");
	}

	uint32_t parent_cluster = 0;
	auto entry = pImpl->find_entry(path, parent_cluster);

	// Read all entries in the parent directory
	auto entries = pImpl->read_dirents(parent_cluster);

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
		throw PS2McError("Entry not found in parent directory");
	}

	// Write all entries back
	pImpl->write_dirents(parent_cluster, entries);
}

uint32_t PS2MemoryCard::getAllocatableSpace()
{
	if (!pImpl->file.is_open())
	{
		throw PS2McError("Memory card not open");
	}

	// Return total usable space on card (excluding system clusters)
	// This is the total clusters available for user data
	uint32_t totalClusters = pImpl->clusters_per_card;
	uint32_t systemClusters = pImpl->allocatable_cluster_offset; // Clusters reserved for FAT, etc.
	return (totalClusters - systemClusters) * pImpl->cluster_size;
}

bool PS2MemoryCard::check()
{
	if (!pImpl->file.is_open())
	{
		throw PS2McError("Memory card not open");
	}

	bool valid = true;

	try
	{
		auto entries = listDir("/");
		for (const auto& entry : entries)
		{
			if (entry.mode & DF_DIR)
			{
				try
				{
					auto subentries = listDir("/" + entry.name);
					for (const auto& subentry : subentries)
					{
						if (subentry.mode & DF_FILE)
						{
							try
							{
								std::string path = "/" + entry.name + "/" + subentry.name;
								auto data = readFile(path);
								(void)data; // Just verify we can read it
							}
							catch (...)
							{
								valid = false;
							}
						}
					}
				}
				catch (...)
				{
					valid = false;
				}
			}
		}
	}
	catch (...)
	{
		valid = false;
	}

	return valid;
}

bool PS2MemoryCard::hasEcc() const
{
	return pImpl->with_ecc;
}

std::string PS2MemoryCard::getPsxTitle(const std::string& savePath)
{
	if (!pImpl->file.is_open())
	{
		throw PS2McError("Memory card not open");
	}

	// Get the save directory mode to check if it's PSX
	auto entry = getEntry(savePath);
	if (!(entry.mode & DF_PSX))
	{
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

			if (data.size() >= 128)
			{
				if (data[0] == 'S' && data[1] == 'C')
				{
					std::string title(reinterpret_cast<const char*>(&data[4]), 64);
					size_t nullPos = title.find('\0');
					if (nullPos != std::string::npos)
						title = title.substr(0, nullPos);
					return ShiftJIS::toUtf8(title);
				}
			}
			break; // Only check first file
		}
	}

	return "";
}

void PS2MemoryCard::saveAs(const std::string& filename, bool withEcc)
{
	if (!pImpl->file.is_open())
	{
		throw PS2McError("Memory card not open");
	}

	if (withEcc == pImpl->with_ecc)
	{
		pImpl->file.seekg(0, std::ios::end);
		size_t fileSize = pImpl->file.tellg();
		pImpl->file.seekg(0, std::ios::beg);

		std::ofstream outFile(filename, std::ios::binary);
		if (!outFile)
		{
			throw PS2McError("Failed to create file: " + filename);
		}

		std::vector<char> buffer(fileSize);
		pImpl->file.read(buffer.data(), fileSize);
		outFile.write(buffer.data(), fileSize);
		outFile.close();
		return;
	}

	PS2MemoryCard newCard;
	newCard.create(filename, 8); // Create standard 8MB card

	uint32_t pageCount = pImpl->clusters_per_card * pImpl->pages_per_cluster;

	newCard.close();

	std::ofstream outFile(filename, std::ios::binary);
	if (!outFile)
	{
		throw PS2McError("Failed to create file: " + filename);
	}

	if (withEcc && !pImpl->with_ecc)
	{
		for (uint32_t i = 0; i < pageCount; ++i)
		{
			auto pageData = pImpl->read_page(i);
			if (pageData.size() < pImpl->page_size)
			{
				pageData.resize(pImpl->page_size, 0);
			}

			outFile.write(reinterpret_cast<const char*>(pageData.data()), pImpl->page_size);

			uint32_t target_spare = divRoundUp(static_cast<int>(pImpl->page_size), 128) * 4;
			auto ecc = eccCalculatePage(pageData, static_cast<int>(pImpl->page_size));
			std::vector<uint8_t> spare(target_spare, 0);
			std::copy(ecc.begin(), ecc.end(), spare.begin());
			outFile.write(reinterpret_cast<const char*>(spare.data()), spare.size());
		}
	}
	else
	{
		for (uint32_t i = 0; i < pageCount; ++i)
		{
			auto pageData = pImpl->read_page(i);
			if (pageData.size() > pImpl->page_size)
			{
				pageData.resize(pImpl->page_size);
			}
			else if (pageData.size() < pImpl->page_size)
			{
				pageData.resize(pImpl->page_size, 0);
			}

			outFile.write(reinterpret_cast<const char*>(pageData.data()), pImpl->page_size);
		}
	}

	outFile.close();
}
