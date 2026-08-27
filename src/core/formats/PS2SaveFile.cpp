// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+
// PS2 save container handling (MAX/EMS/SharkPort/CodeBreaker/PSV) follows the mymc++ / mymc implementations and PS2 save format documentation.

#include "PS2SaveFile.h"
#include "common/Logger.h"
#include "common/round.h"
#include "common/lzari.h"
#include "common/sjis.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <zlib.h>

namespace
{
	inline bool modeIsDir(uint16_t mode)
	{
		return (mode & DF_DIR) != 0;
	}

	inline bool modeIsFile(uint16_t mode)
	{
		return (mode & DF_FILE) != 0;
	}
} // namespace

class PS2SaveFile::Impl
{
public:
	Error& m_error;
	std::vector<PS2SaveEntry> m_entries;
	std::string m_title;
	SaveFormat m_format;

	explicit Impl(Error& error)
		: m_error(error)
		, m_format(SaveFormat::UNKNOWN)
	{
	}

	bool readBytes(std::ifstream& f, void* dest, size_t n)
	{
		f.read(reinterpret_cast<char*>(dest), n);
		if (static_cast<size_t>(f.gcount()) != n)
		{
			return m_error.Fail("Unexpected EOF");
		}
		return true;
	}

	bool readFixed(std::ifstream& f, size_t n, std::vector<uint8_t>& out)
	{
		out.resize(n);
		return readBytes(f, out.data(), n);
	}

	bool loadEms(std::ifstream& file);
	bool loadMaxDrive(std::ifstream& file);
	bool loadSharkPort(std::ifstream& file);
	bool loadCodeBreaker(std::ifstream& file);
	bool loadPsv(std::ifstream& file);
	bool saveEms(std::ofstream& file);
	bool saveMaxDrive(std::ofstream& file);
};

PS2SaveFile::PS2SaveFile()
	: m_impl(std::make_unique<Impl>(m_error))
{
}

PS2SaveFile::~PS2SaveFile() = default;

bool PS2SaveFile::load(const std::string& filename)
{
	m_error.Clear();
	std::ifstream file(filename, std::ios::binary);
	if (!file)
	{
		return m_error.Fail("Failed to open: " + filename);
	}

	m_impl->m_format = detectFormat(filename);

	switch (m_impl->m_format)
	{
		case SaveFormat::MAX_DRIVE:
			return m_impl->loadMaxDrive(file);
		case SaveFormat::EMS:
			return m_impl->loadEms(file);
		case SaveFormat::SHARKPORT:
		case SaveFormat::XPORT: // X-Port uses same format as SharkPort
			return m_impl->loadSharkPort(file);
		case SaveFormat::CODEBREAKER:
			return m_impl->loadCodeBreaker(file);
		case SaveFormat::PSV:
			return m_impl->loadPsv(file);
		default:
			return m_error.Fail("Unknown save format");
	}
}

bool PS2SaveFile::save(const std::string& filename, SaveFormat format)
{
	m_error.Clear();
	std::ofstream file(filename, std::ios::binary);
	if (!file)
	{
		return m_error.Fail("Failed to create: " + filename);
	}

	if (format == SaveFormat::UNKNOWN)
	{
		format = m_impl->m_format;
	}

	switch (format)
	{
		case SaveFormat::MAX_DRIVE:
			return m_impl->saveMaxDrive(file);
		case SaveFormat::EMS:
			return m_impl->saveEms(file);
		default:
			return m_error.Fail("Unsupported save format for writing");
	}
}

std::vector<PS2SaveEntry>& PS2SaveFile::getEntries()
{
	return m_impl->m_entries;
}

const std::vector<PS2SaveEntry>& PS2SaveFile::getEntries() const
{
	return m_impl->m_entries;
}

std::string PS2SaveFile::getTitle() const
{
	return m_impl->m_title;
}

void PS2SaveFile::setTitle(const std::string& title)
{
	m_impl->m_title = title;
}

SaveFormat PS2SaveFile::getFormat() const
{
	return m_impl->m_format;
}

SaveFormat PS2SaveFile::detectFormat(const std::string& filename)
{
	std::ifstream file(filename, std::ios::binary);
	if (!file)
	{
		return SaveFormat::UNKNOWN;
	}

	char magic[16];
	file.read(magic, 16);

	if (std::memcmp(magic, PS2SAVE_MAX_MAGIC, 12) == 0)
	{
		return SaveFormat::MAX_DRIVE;
	}
	if (std::memcmp(magic, PS2SAVE_SPS_MAGIC, 13) == 0)
	{
		size_t dotPos = filename.find_last_of('.');
		if (dotPos != std::string::npos)
		{
			std::string ext = filename.substr(dotPos);
			std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
				return static_cast<char>(std::tolower(ch));
			});
			if (ext == ".xps")
			{
				return SaveFormat::XPORT;
			}
		}
		return SaveFormat::SHARKPORT;
	}
	if (std::memcmp(magic, PS2SAVE_CBS_MAGIC, 4) == 0)
	{
		return SaveFormat::CODEBREAKER;
	}
	if (std::memcmp(magic, PS2SAVE_PSV_MAGIC, 4) == 0)
	{
		return SaveFormat::PSV;
	}
	if (std::memcmp(magic, PS2SAVE_NPO_MAGIC, 5) == 0)
	{
		return SaveFormat::NPORT;
	}

	size_t dotPos = filename.find_last_of('.');
	if (dotPos != std::string::npos)
	{
		std::string ext = filename.substr(dotPos);
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
			return static_cast<char>(std::tolower(ch));
		});

		if (ext == ".psu")
		{
			return SaveFormat::EMS;
		}
		if (ext == ".max")
		{
			return SaveFormat::MAX_DRIVE;
		}
		if (ext == ".sps")
		{
			return SaveFormat::SHARKPORT;
		}
		if (ext == ".cbs")
		{
			return SaveFormat::CODEBREAKER;
		}
		if (ext == ".xps")
		{
			return SaveFormat::XPORT;
		}
		if (ext == ".psv")
		{
			return SaveFormat::PSV;
		}
	}

	return SaveFormat::UNKNOWN;
}

std::string PS2SaveFile::formatToExtension(SaveFormat format)
{
	switch (format)
	{
		case SaveFormat::MAX_DRIVE:
		case SaveFormat::EMS:
			return ".psu";
		case SaveFormat::SHARKPORT:
			return ".sps";
		case SaveFormat::CODEBREAKER:
			return ".cbs";
		case SaveFormat::XPORT:
			return ".xps";
		case SaveFormat::PSV:
			return ".psv";
		case SaveFormat::NPORT:
			return ".npo";
		default:
			return ".dat";
	}
}

bool PS2SaveFile::Impl::loadMaxDrive(std::ifstream& file)
{
	char hdr[0x5C];
	if (!readBytes(file, hdr, 0x5C))
	{
		return m_error.Fail("Corrupt MAX header");
	}
	struct Hdr
	{
		char magic[12];
		uint32_t crc;
		char dirname[32];
		char iconsys[32];
		uint32_t compressedLen;
		uint32_t dirLen;
		uint32_t length;
	};
	const Hdr* h = reinterpret_cast<const Hdr*>(hdr);
	if (std::memcmp(h->magic, PS2SAVE_MAX_MAGIC, 12) != 0)
	{
		return m_error.Fail("Not a MAX Drive save");
	}
	std::string dirname = zeroTerminate(std::string(h->dirname, 32));
	uint32_t compressedLen = h->compressedLen;
	uint32_t dirLen = h->dirLen;
	uint32_t length = h->length;

	std::vector<uint8_t> compressed;
	if (compressedLen == length)
	{
		file.seekg(0, std::ios::end);
		size_t remaining = static_cast<size_t>(file.tellg()) - 0x5C;
		file.seekg(0x5C, std::ios::beg);
		if (!readFixed(file, remaining, compressed))
		{
			return false;
		}
	}
	else
	{
		if (!readFixed(file, compressedLen - 4, compressed)) // -4 because crc already read
		{
			return false;
		}
	}

	auto decompressed = lzariDecompress(compressed, length);
	if (decompressed.size() != length)
	{
		return m_error.Fail("MAX decompress size mismatch");
	}

	m_entries.clear();
	m_entries.reserve(dirLen);
	size_t off = 0;
	auto timestamp = time(nullptr);
	for (uint32_t i = 0; i < dirLen; ++i)
	{
		if (off + 36 > decompressed.size())
		{
			return m_error.Fail("MAX directory truncated");
		}
		uint32_t l = *reinterpret_cast<const uint32_t*>(&decompressed[off]);
		std::string name(reinterpret_cast<const char*>(&decompressed[off + 4]), 32);
		name = zeroTerminate(name);
		off += 36;
		if (off + l > decompressed.size())
		{
			return m_error.Fail("MAX file truncated");
		}
		std::vector<uint8_t> data(decompressed.begin() + off, decompressed.begin() + off + l);
		off += l;
		off = roundUp(static_cast<int>(off) + 8, 16) - 8;

		PS2McDirEntry ent{};
		ent.mode = DF_RWX | DF_FILE | DF_0400 | DF_EXISTS;
		ent.unused = 0;
		ent.length = l;
		ent.created = timeToTod(timestamp);
		ent.cluster = 0;
		ent.dirEntry = 0;
		ent.modified = ent.created;
		ent.attr = 0;
		ent.name = name;
		m_entries.push_back({ent, std::move(data)});
	}
	m_title = dirname;
	m_format = SaveFormat::MAX_DRIVE;
	return true;
}

bool PS2SaveFile::Impl::loadSharkPort(std::ifstream& file)
{
	const std::string fmtName = (m_format == SaveFormat::XPORT) ? "X-Port" : "SharkPort";

	std::vector<uint8_t> magic;
	if (!readFixed(file, 17, magic))
	{
		return false;
	}
	if (std::memcmp(magic.data(), PS2SAVE_SPS_MAGIC, 13) != 0)
	{
		return m_error.Fail("Not a " + fmtName + " save");
	}

	uint32_t saveType = 0;
	if (!readBytes(file, &saveType, 4))
	{
		return false;
	}

	uint32_t dirNameLen = 0;
	if (!readBytes(file, &dirNameLen, 4))
	{
		return false;
	}
	std::vector<uint8_t> dirNameData;
	if (!readFixed(file, dirNameLen, dirNameData))
	{
		return false;
	}
	std::string dirname(dirNameData.begin(), dirNameData.end());

	uint32_t dateStampLen = 0;
	if (!readBytes(file, &dateStampLen, 4))
	{
		return false;
	}
	std::vector<uint8_t> unused;
	if (!readFixed(file, dateStampLen, unused))
	{
		return false;
	}

	uint32_t commentLen = 0;
	if (!readBytes(file, &commentLen, 4))
	{
		return false;
	}
	if (!readFixed(file, commentLen, unused))
	{
		return false;
	}

	uint32_t fileLen = 0;
	if (!readBytes(file, &fileLen, 4))
	{
		return false;
	}

	uint16_t headerLen = 0;
	if (!readBytes(file, &headerLen, 2))
	{
		return false;
	}

	std::vector<uint8_t> nameBuf(64);
	if (!readBytes(file, nameBuf.data(), 64))
	{
		return false;
	}

	uint32_t dirLen = 0;
	if (!readBytes(file, &dirLen, 4))
	{
		return false;
	}

	if (!readFixed(file, 8, unused)) // skip 8 bytes
	{
		return false;
	}

	uint16_t dirMode = 0;
	if (!readBytes(file, &dirMode, 2))
	{
		return false;
	}

	if (!readFixed(file, 2, unused)) // skip 2 bytes
	{
		return false;
	}

	std::vector<uint8_t> createdData;
	if (!readFixed(file, 8, createdData))
	{
		return false;
	}

	std::vector<uint8_t> modifiedData;
	if (!readFixed(file, 8, modifiedData))
	{
		return false;
	}

	if (headerLen > 98)
	{
		if (!readFixed(file, headerLen - 98, unused))
		{
			return false;
		}
	}

	dirMode = ((dirMode / 256) % 256) + ((dirMode % 256) * 256);

	if (!(dirMode & DF_DIR) || dirLen < 2)
	{
		return m_error.Fail("Invalid " + fmtName + " directory header");
	}

	m_entries.clear();
	m_entries.reserve(dirLen - 2);

	for (uint32_t i = 0; i < dirLen - 2; ++i)
	{
		uint16_t fileHeaderLen = 0;
		if (!readBytes(file, &fileHeaderLen, 2))
		{
			return false;
		}

		std::vector<uint8_t> fileNameBuf(64);
		if (!readBytes(file, fileNameBuf.data(), 64))
		{
			return false;
		}
		std::string fname(reinterpret_cast<const char*>(fileNameBuf.data()));
		fname = zeroTerminate(fname);

		uint32_t fileSize = 0;
		if (!readBytes(file, &fileSize, 4))
		{
			return false;
		}
		if (!readFixed(file, 8, unused)) // skip 8 bytes
		{
			return false;
		}

		uint16_t fileMode = 0;
		if (!readBytes(file, &fileMode, 2))
		{
			return false;
		}
		if (!readFixed(file, 2, unused)) // skip 2 bytes
		{
			return false;
		}

		std::vector<uint8_t> fileCreatedData;
		if (!readFixed(file, 8, fileCreatedData))
		{
			return false;
		}
		PS2McTod fileCreated = unpackTod(fileCreatedData);

		std::vector<uint8_t> fileModifiedData;
		if (!readFixed(file, 8, fileModifiedData))
		{
			return false;
		}
		PS2McTod fileModified = unpackTod(fileModifiedData);

		if (fileHeaderLen > 98)
		{
			if (!readFixed(file, fileHeaderLen - 98, unused))
			{
				return false;
			}
		}

		fileMode = ((fileMode / 256) % 256) + ((fileMode % 256) * 256);

		if (!(fileMode & DF_FILE))
		{
			return m_error.Fail("Non-file in " + fmtName + " directory");
		}

		std::vector<uint8_t> fileData;
		if (!readFixed(file, fileSize, fileData))
		{
			return false;
		}

		PS2McDirEntry ent{};
		ent.mode = fileMode | DF_EXISTS;
		ent.length = fileSize;
		ent.name = fname;
		ent.created = fileCreated;
		ent.modified = fileModified;

		m_entries.push_back({ent, std::move(fileData)});
	}

	m_title = dirname;
	if (m_format != SaveFormat::XPORT)
	{
		m_format = SaveFormat::SHARKPORT;
	}
	return true;
}

namespace
{
	// clang-format off
	static const uint8_t s_cbsRc4Key[] = {
		0x5f, 0x1f, 0x85, 0x6f, 0x31, 0xaa, 0x3b, 0x18,
		0x21, 0xb9, 0xce, 0x1c, 0x07, 0x4c, 0x9c, 0xb4,
		0x81, 0xb8, 0xef, 0x98, 0x59, 0xae, 0xf9, 0x26,
		0xe3, 0x80, 0xa3, 0x29, 0x2d, 0x73, 0x51, 0x62,
		0x7c, 0x64, 0x46, 0xf4, 0x34, 0x1a, 0xf6, 0xe1,
		0xba, 0x3a, 0x0d, 0x82, 0x79, 0x0a, 0x5c, 0x16,
		0x71, 0x49, 0x8e, 0xac, 0x8c, 0x9f, 0x35, 0x19,
		0x45, 0x94, 0x3f, 0x56, 0x0c, 0x91, 0x00, 0x0b,
		0xd7, 0xb0, 0xdd, 0x39, 0x66, 0xa1, 0x76, 0x52,
		0x13, 0x57, 0xf3, 0xbb, 0x4e, 0xe5, 0xdc, 0xf0,
		0x65, 0x84, 0xb2, 0xd6, 0xdf, 0x15, 0x3c, 0x63,
		0x1d, 0x89, 0x14, 0xbd, 0xd2, 0x36, 0xfe, 0xb1,
		0xca, 0x8b, 0xa4, 0xc6, 0x9e, 0x67, 0x47, 0x37,
		0x42, 0x6d, 0x6a, 0x03, 0x92, 0x70, 0x05, 0x7d,
		0x96, 0x2f, 0x40, 0x90, 0xc4, 0xf1, 0x3e, 0x3d,
		0x01, 0xf7, 0x68, 0x1e, 0xc3, 0xfc, 0x72, 0xb5,
		0x54, 0xcf, 0xe7, 0x41, 0xe4, 0x4d, 0x83, 0x55,
		0x12, 0x22, 0x09, 0x78, 0xfa, 0xde, 0xa7, 0x06,
		0x08, 0x23, 0xbf, 0x0f, 0xcc, 0xc1, 0x97, 0x61,
		0xc5, 0x4a, 0xe6, 0xa0, 0x11, 0xc2, 0xea, 0x74,
		0x02, 0x87, 0xd5, 0xd1, 0x9d, 0xb7, 0x7e, 0x38,
		0x60, 0x53, 0x95, 0x8d, 0x25, 0x77, 0x10, 0x5e,
		0x9b, 0x7f, 0xd8, 0x6e, 0xda, 0xa2, 0x2e, 0x20,
		0x4f, 0xcd, 0x8f, 0xcb, 0xbe, 0x5a, 0xe0, 0xed,
		0x2c, 0x9a, 0xd4, 0xe2, 0xaf, 0xd0, 0xa9, 0xe8,
		0xad, 0x7a, 0xbc, 0xa8, 0xf2, 0xee, 0xeb, 0xf5,
		0xa6, 0x99, 0x28, 0x24, 0x6c, 0x2b, 0x75, 0x5d,
		0xf8, 0xd3, 0x86, 0x17, 0xfb, 0xc0, 0x7b, 0xb3,
		0x58, 0xdb, 0xc7, 0x4b, 0xff, 0x04, 0x50, 0xe9,
		0x88, 0x69, 0xc9, 0x2a, 0xab, 0xfd, 0x5b, 0x1b,
		0x8a, 0xd9, 0xec, 0x27, 0x44, 0x0e, 0x33, 0xc8,
		0x6b, 0x93, 0x32, 0x48, 0xb6, 0x30, 0x43, 0xa5};
	// clang-format on

	std::vector<uint8_t> rc4Crypt(const std::vector<uint8_t>& s, const std::vector<uint8_t>& data)
	{
		std::vector<uint8_t> sCopy = s;
		std::vector<uint8_t> result = data;

		size_t i = 0, j = 0;
		for (size_t n = 0; n < data.size(); ++n)
		{
			i = (i + 1) % 256;
			j = (j + sCopy[i]) % 256;
			std::swap(sCopy[i], sCopy[j]);
			uint8_t k = sCopy[(sCopy[i] + sCopy[j]) % 256];
			result[n] ^= k;
		}
		return result;
	}
} // namespace

bool PS2SaveFile::Impl::loadCodeBreaker(std::ifstream& file)
{
	std::vector<uint8_t> magic;
	if (!readFixed(file, 4, magic))
	{
		return false;
	}
	if (std::memcmp(magic.data(), PS2SAVE_CBS_MAGIC, 4) != 0)
	{
		return m_error.Fail("Not a CodeBreaker save");
	}

	uint32_t headerReserved = 0, headerLen = 0;
	if (!readBytes(file, &headerReserved, 4) || !readBytes(file, &headerLen, 4))
	{
		return false;
	}

	if (headerLen < 124)
	{
		return m_error.Fail("CodeBreaker header too short");
	}

	std::vector<uint8_t> header;
	if (!readFixed(file, headerLen - 12, header))
	{
		return false;
	}

	uint32_t decompressedLen = 0, fileLen = 0;
	std::memcpy(&decompressedLen, header.data() + 0, 4);
	std::memcpy(&fileLen, header.data() + 4, 4);

	char dirname[32];
	std::memcpy(dirname, header.data() + 8, 32);

	std::vector<uint8_t> createdData(header.begin() + 40, header.begin() + 48);
	PS2McTod created = unpackTod(createdData);

	std::vector<uint8_t> modifiedData(header.begin() + 48, header.begin() + 56);
	PS2McTod modified = unpackTod(modifiedData);

	uint32_t dirMode = 0;
	std::memcpy(&dirMode, header.data() + 64, 4);

	if (!(dirMode & DF_DIR))
	{
		dirMode = DF_DIR | DF_RWX | DF_0400;
	}

	if (todToTime(created) == 0)
	{
		created = timeToTod(std::time(nullptr));
	}
	if (todToTime(modified) == 0)
	{
		modified = timeToTod(std::time(nullptr));
	}

	size_t remaining = 0;
	{
		auto cur = file.tellg();
		file.seekg(0, std::ios::end);
		remaining = static_cast<size_t>(file.tellg() - cur);
		file.seekg(cur, std::ios::beg);
	}

	size_t bodyLen = fileLen;
	if (bodyLen > remaining)
	{
		bodyLen = remaining;
	}
	if (bodyLen != fileLen && bodyLen != fileLen - headerLen)
	{
		return m_error.Fail("Unexpected EOF");
	}

	std::vector<uint8_t> body;
	if (!readFixed(file, bodyLen, body))
	{
		return false;
	}

	std::vector<uint8_t> rc4Key(std::begin(s_cbsRc4Key), std::end(s_cbsRc4Key));
	auto decrypted = rc4Crypt(rc4Key, body);

	std::vector<uint8_t> decompressed(decompressedLen);
	z_stream stream{};
	stream.avail_in = static_cast<uInt>(decrypted.size());
	stream.next_in = decrypted.data();
	stream.avail_out = static_cast<uInt>(decompressedLen);
	stream.next_out = decompressed.data();

	if (inflateInit(&stream) != Z_OK || inflate(&stream, Z_FINISH) != Z_STREAM_END)
	{
		return m_error.Fail("CodeBreaker zlib decompress failed");
	}
	inflateEnd(&stream);

	m_entries.clear();
	size_t off = 0;

	while (off < decompressed.size())
	{
		if (off + 64 > decompressed.size())
		{
			break;
		}

		uint32_t fileSize = 0;
		std::memcpy(&fileSize, decompressed.data() + off + 16, 4);
		uint16_t fileMode = 0;
		std::memcpy(&fileMode, decompressed.data() + off + 20, 2);

		std::vector<uint8_t> fileCreatedData(decompressed.begin() + off + 0, decompressed.begin() + off + 8);
		std::vector<uint8_t> fileModifiedData(decompressed.begin() + off + 8, decompressed.begin() + off + 16);

		char fname[32];
		std::memcpy(fname, decompressed.data() + off + 32, 32);

		off += 64;
		if (off + fileSize > decompressed.size())
		{
			break;
		}

		PS2McTod fileCreated = unpackTod(fileCreatedData);
		PS2McTod fileModified = unpackTod(fileModifiedData);

		if (!(fileMode & DF_FILE))
		{
			return m_error.Fail("Non-file in CodeBreaker save");
		}

		std::vector<uint8_t> fileData(decompressed.begin() + off, decompressed.begin() + off + fileSize);
		off += fileSize;

		PS2McDirEntry ent{};
		ent.mode = fileMode | DF_EXISTS;
		ent.length = fileSize;
		ent.name = zeroTerminate(std::string(fname, 32));
		ent.created = fileCreated;
		ent.modified = fileModified;

		m_entries.push_back({ent, std::move(fileData)});
	}

	m_title = zeroTerminate(std::string(dirname, 32));
	m_format = SaveFormat::CODEBREAKER;
	return true;
}

bool PS2SaveFile::Impl::loadPsv(std::ifstream& file)
{
	std::vector<uint8_t> magicData;
	if (!readFixed(file, 4, magicData))
	{
		return false;
	}
	if (std::memcmp(magicData.data(), PS2SAVE_PSV_MAGIC, 4) != 0)
	{
		return m_error.Fail("Not a PSV file");
	}

	uint32_t version = 0, nextSectionSize = 0, saveType = 0;
	if (!readBytes(file, &version, 4))
	{
		return false;
	}
	std::vector<uint8_t> unused;
	if (!readFixed(file, 40, unused)) // signature
	{
		return false;
	}
	if (!readFixed(file, 8, unused)) // reserved
	{
		return false;
	}
	if (!readBytes(file, &nextSectionSize, 4))
	{
		return false;
	}
	if (!readBytes(file, &saveType, 4))
	{
		return false;
	}

	if (version != 0)
	{
		return m_error.Fail("Unsupported PSV version");
	}

	if (saveType != 2 && saveType != 1)
	{
		return m_error.Fail("Unsupported PSV save type");
	}

	if (saveType == 2)
	{
		uint32_t unusedField = 0;
		uint32_t sysPos = 0, sysSize = 0;
		uint32_t icon1Pos = 0, icon1Size = 0, icon2Pos = 0, icon2Size = 0;
		uint32_t icon3Pos = 0, icon3Size = 0, filesCount = 0;

		if (!readBytes(file, &unusedField, 4) ||
			!readBytes(file, &sysPos, 4) ||
			!readBytes(file, &sysSize, 4) ||
			!readBytes(file, &icon1Pos, 4) ||
			!readBytes(file, &icon1Size, 4) ||
			!readBytes(file, &icon2Pos, 4) ||
			!readBytes(file, &icon2Size, 4) ||
			!readBytes(file, &icon3Pos, 4) ||
			!readBytes(file, &icon3Size, 4) ||
			!readBytes(file, &filesCount, 4))
		{
			return false;
		}

		std::vector<uint8_t> rootCreatedData;
		if (!readFixed(file, 8, rootCreatedData))
		{
			return false;
		}
		std::vector<uint8_t> rootModifiedData;
		if (!readFixed(file, 8, rootModifiedData))
		{
			return false;
		}
		uint32_t rootSize = 0;
		uint32_t rootMode = 0;
		if (!readBytes(file, &rootSize, 4) || !readBytes(file, &rootMode, 4))
		{
			return false;
		}

		std::vector<uint8_t> rootFilename(32);
		if (!readBytes(file, rootFilename.data(), 32))
		{
			return false;
		}

		if (!(rootMode & DF_DIR))
		{
			return m_error.Fail("PSV root is not a directory");
		}

		m_title = zeroTerminate(std::string(reinterpret_cast<const char*>(rootFilename.data()), 32));

		m_entries.clear();
		m_entries.reserve(filesCount);

		std::vector<std::pair<uint32_t, PS2McDirEntry>> fileList;
		for (uint32_t i = 0; i < filesCount; ++i)
		{
			std::vector<uint8_t> fileCreatedData;
			if (!readFixed(file, 8, fileCreatedData))
			{
				return false;
			}
			std::vector<uint8_t> fileModifiedData;
			if (!readFixed(file, 8, fileModifiedData))
			{
				return false;
			}
			uint32_t fileSize = 0;
			uint32_t fileMode = 0;
			if (!readBytes(file, &fileSize, 4) || !readBytes(file, &fileMode, 4))
			{
				return false;
			}

			std::vector<uint8_t> filenameBuf(32);
			if (!readBytes(file, filenameBuf.data(), 32))
			{
				return false;
			}

			uint32_t fileOffset = 0;
			if (!readBytes(file, &fileOffset, 4))
			{
				return false;
			}

			PS2McDirEntry ent{};
			ent.mode = static_cast<uint16_t>(fileMode | DF_EXISTS);
			ent.length = fileSize;
			ent.name = zeroTerminate(std::string(reinterpret_cast<const char*>(filenameBuf.data()), 32));
			ent.created = unpackTod(fileCreatedData);
			ent.modified = unpackTod(fileModifiedData);

			if (fileMode & DF_FILE)
			{
				fileList.push_back({fileOffset, ent});
			}
		}

		for (const auto& [offset, ent] : fileList)
		{
			file.seekg(offset);
			std::vector<uint8_t> fileData;
			if (!readFixed(file, ent.length, fileData))
			{
				return false;
			}
			m_entries.push_back({ent, std::move(fileData)});
		}
	}
	else
	{
		uint32_t saveSize = 0, saveOffset = 0;
		if (!readBytes(file, &saveSize, 4) || !readBytes(file, &saveOffset, 4))
		{
			return false;
		}
		if (!readFixed(file, 20, unused)) // reserved
		{
			return false;
		}
		uint32_t unusedNext = 0;
		if (!readBytes(file, &unusedNext, 4))
		{
			return false;
		}
		if (!readFixed(file, 4, unused)) // reserved
		{
			return false;
		}

		std::vector<uint8_t> prodCode(20);
		if (!readBytes(file, prodCode.data(), 20))
		{
			return false;
		}

		m_title = zeroTerminate(std::string(reinterpret_cast<const char*>(prodCode.data()), 20));

		m_entries.clear();
		uint16_t mode = DF_RWX | DF_FILE | DF_0080 | DF_0400 | DF_EXISTS;

		PS2McDirEntry ent{};
		ent.mode = mode;
		ent.length = saveSize;
		ent.name = "SAVEGAME.PSX";
		ent.created = ent.modified = timeToTod(std::time(nullptr));

		file.seekg(saveOffset);
		std::vector<uint8_t> fileData;
		if (!readFixed(file, saveSize, fileData))
		{
			return false;
		}
		m_entries.push_back({ent, std::move(fileData)});
	}

	m_format = SaveFormat::PSV;
	return true;
}

bool PS2SaveFile::Impl::saveMaxDrive(std::ofstream& file)
{
	std::vector<uint8_t> blob;
	const bool hasDirHeader = !m_entries.empty() && (m_entries[0].dirEntry.mode & DF_DIR);
	const size_t firstFileIdx = hasDirHeader ? 1 : 0;

	for (size_t i = firstFileIdx; i < m_entries.size(); ++i)
	{
		const auto& e = m_entries[i];
		const auto& ent = e.dirEntry;
		const auto& data = e.data;
		uint32_t len = static_cast<uint32_t>(data.size());
		char name[32] = {0};
		const size_t copyLen = std::min(ent.name.size(), sizeof(name) - 1);
		std::memcpy(name, ent.name.c_str(), copyLen);
		name[copyLen] = '\0';
		size_t cur = blob.size();
		blob.resize(cur + 36 + len);
		std::memcpy(blob.data() + cur, &len, 4);
		std::memcpy(blob.data() + cur + 4, name, 32);
		std::memcpy(blob.data() + cur + 36, data.data(), len);
		blob.resize(roundUp(static_cast<int>(cur + 36 + len) + 8, 16) - 8);
	}
	uint32_t length = static_cast<uint32_t>(blob.size());
	auto compressed = lzariCompress(blob);

	char dirname[32] = {0};
	if (!m_entries.empty())
	{
		std::string dName = hasDirHeader ? m_entries[0].dirEntry.name : m_title;
		if (dName.empty())
		{
			dName = m_entries[0].dirEntry.name;
		}
		const size_t copyLen = std::min(dName.size(), sizeof(dirname) - 1);
		std::memcpy(dirname, dName.c_str(), copyLen);
		dirname[copyLen] = '\0';
	}
	char iconsys[32] = {0};
	uint32_t compressedLen = static_cast<uint32_t>(compressed.size()) + 4; // include CRC field itself
	uint32_t dirLen = static_cast<uint32_t>(hasDirHeader ? m_entries.size() - 1 : m_entries.size());
	uint32_t crc = 0; // omitted
	file.write(PS2SAVE_MAX_MAGIC, 12);
	file.write(reinterpret_cast<const char*>(&crc), 4);
	file.write(dirname, 32);
	file.write(iconsys, 32);
	file.write(reinterpret_cast<const char*>(&compressedLen), 4);
	file.write(reinterpret_cast<const char*>(&dirLen), 4);
	file.write(reinterpret_cast<const char*>(&length), 4);
	file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
	file.flush();
	if (!file)
	{
		return m_error.Fail("Failed to write MAX save");
	}
	return true;
}

bool PS2SaveFile::Impl::loadEms(std::ifstream& file)
{
	std::vector<uint8_t> direntRaw, dotRaw, dotdotRaw;
	if (!readFixed(file, PS2MC_DIRENT_LENGTH, direntRaw) ||
		!readFixed(file, PS2MC_DIRENT_LENGTH, dotRaw) ||
		!readFixed(file, PS2MC_DIRENT_LENGTH, dotdotRaw))
	{
		return false;
	}
	auto dirent = unpackDirEntry(direntRaw);
	auto dotent = unpackDirEntry(dotRaw);
	auto dotdot = unpackDirEntry(dotdotRaw);
	if (!modeIsDir(dirent.mode) || !modeIsDir(dotent.mode) || !modeIsDir(dotdot.mode) || dirent.length < 2)
	{
		return m_error.Fail("Not a valid PSU file");
	}
	uint32_t fileCount = dirent.length - 2;
	m_entries.clear();
	m_entries.reserve(static_cast<size_t>(fileCount) + 1);
	PS2SaveEntry dirSlot;
	dirSlot.dirEntry = dirent;
	dirSlot.data.clear();
	m_entries.push_back(std::move(dirSlot));
	for (uint32_t i = 0; i < fileCount; ++i)
	{
		std::vector<uint8_t> entRaw;
		if (!readFixed(file, PS2MC_DIRENT_LENGTH, entRaw))
		{
			return false;
		}
		auto ent = unpackDirEntry(entRaw);
		if (!modeIsFile(ent.mode))
		{
			const std::string name = ent.name.empty() ? std::string("<unnamed>") : ent.name;
			Logger::warn("Skipping subdir (entry {}): {}", i, name);
			continue;
		}
		std::vector<uint8_t> data;
		if (!readFixed(file, ent.length, data))
		{
			return false;
		}
		size_t pad = roundUp(static_cast<int>(ent.length), 1024) - ent.length;
		if (pad)
		{
			std::vector<uint8_t> padData;
			if (!readFixed(file, pad, padData))
			{
				return false;
			}
		}
		m_entries.push_back({ent, std::move(data)});
	}
	m_format = SaveFormat::EMS;
	m_title = dirent.name;
	return true;
}

bool PS2SaveFile::Impl::saveEms(std::ofstream& file)
{
	const bool hasDirHeader = !m_entries.empty() && (m_entries[0].dirEntry.mode & DF_DIR);
	const size_t numFiles = hasDirHeader ? m_entries.size() - 1 : m_entries.size();

	PS2McDirEntry root{};
	if (hasDirHeader)
	{
		root = m_entries[0].dirEntry;
	}
	else
	{
		root.name = m_entries.empty() ? "SAVE" : m_entries[0].dirEntry.name;
		root.created = timeToTod(time(nullptr));
		root.modified = root.created;
	}
	root.mode = DF_RWX | DF_DIR | DF_0400 | DF_EXISTS;
	root.length = static_cast<uint32_t>(numFiles + 2);

	file.write(reinterpret_cast<const char*>(packDirEntry(root).data()), PS2MC_DIRENT_LENGTH);

	PS2McDirEntry dot = root;
	dot.name = ".";
	file.write(reinterpret_cast<const char*>(packDirEntry(dot).data()), PS2MC_DIRENT_LENGTH);
	PS2McDirEntry dotdot = root;
	dotdot.name = "..";
	file.write(reinterpret_cast<const char*>(packDirEntry(dotdot).data()), PS2MC_DIRENT_LENGTH);

	for (size_t idx = hasDirHeader ? 1 : 0; idx < m_entries.size(); ++idx)
	{
		const auto& e = m_entries[idx];
		auto ent = e.dirEntry;
		ent.mode = DF_RWX | DF_FILE | DF_0400 | DF_EXISTS;
		auto packed = packDirEntry(ent);
		file.write(reinterpret_cast<const char*>(packed.data()), packed.size());
		file.write(reinterpret_cast<const char*>(e.data.data()), e.data.size());
		size_t pad = roundUp(static_cast<int>(e.data.size()), 1024) - e.data.size();
		if (pad)
		{
			std::vector<char> zeros(pad, 0);
			file.write(zeros.data(), pad);
		}
	}
	file.flush();
	if (!file)
	{
		return m_error.Fail("Failed to write PSU save");
	}
	return true;
}

std::string sjisToUtf8(const std::string& sjis)
{
	return ShiftJIS::toUtf8(sjis);
}

std::string utf8ToSjis(const std::string& utf8)
{
	return ShiftJIS::fromUtf8(utf8);
}

std::wstring sjisToWide(const std::string& sjis)
{
	std::string utf8 = ShiftJIS::toUtf8(sjis);
	std::wstring result;
	result.resize(utf8.size());
	for (size_t i = 0; i < utf8.size(); ++i)
	{
		result[i] = static_cast<wchar_t>(static_cast<unsigned char>(utf8[i]));
	}
	return result;
}

std::string wideToSjis(const std::wstring& wide)
{
	std::string utf8;
	utf8.reserve(wide.size());
	for (wchar_t ch : wide)
	{
		if (ch < 0x80)
		{
			utf8 += static_cast<char>(ch);
		}
		else
		{
			utf8 += '?';
		}
	}
	return ShiftJIS::fromUtf8(utf8);
}

std::string makeLongname(const std::string& dirname, const PS2SaveFile& save)
{
	std::string title = save.getTitle();

	uint32_t crc = 0xFFFFFFFF;
	const auto& entries = save.getEntries();
	const bool hasDirHeader = !entries.empty() && (entries[0].dirEntry.mode & DF_DIR);
	const size_t firstFileIdx = hasDirHeader ? 1 : 0;
	for (size_t i = firstFileIdx; i < entries.size(); ++i)
	{
		for (uint8_t byte : entries[i].data)
		{
			crc ^= byte;
			for (int bit = 0; bit < 8; ++bit)
			{
				if (crc & 1)
				{
					crc = (crc >> 1) ^ 0xEDB88320;
				}
				else
				{
					crc = crc >> 1;
				}
			}
		}
	}
	crc ^= 0xFFFFFFFF;

	std::string dirPrefix = dirname;
	if (dirPrefix.length() >= 12 &&
		(dirPrefix.substr(0, 2) == "BA" || dirPrefix.substr(0, 2) == "BJ" ||
			dirPrefix.substr(0, 2) == "BE" || dirPrefix.substr(0, 2) == "BK"))
	{
		if (dirPrefix.substr(2, 4) == "DATA")
		{
			title = ""; // Clear title for DATA directories
		}
		else
		{
			dirPrefix = dirPrefix.substr(2, 10);
		}
	}

	std::string cleanTitle;
	bool lastWasSpace = true;
	for (char c : title)
	{
		if (std::isspace(static_cast<unsigned char>(c)))
		{
			if (!lastWasSpace)
			{
				cleanTitle += ' ';
				lastWasSpace = true;
			}
		}
		else
		{
			cleanTitle += c;
			lastWasSpace = false;
		}
	}
	if (!cleanTitle.empty() && cleanTitle.back() == ' ')
	{
		cleanTitle.pop_back();
	}

	auto fixChar = [](char c) -> char {
		unsigned char uc = static_cast<unsigned char>(c);
		if (uc < 32 || uc >= 127)
		{
			return '_';
		}
		if (c == '<' || c == '>')
		{
			return '(';
		}
		if (c == ':' || c == '"' || c == '/' || c == '\\' || c == '|')
		{
			return '_';
		}
		if (c == '?' || c == '*')
		{
			return '_';
		}
		return c;
	};

	std::string fixedTitle;
	for (char c : cleanTitle)
	{
		fixedTitle += fixChar(c);
	}

	char crcStr[16];
	std::snprintf(crcStr, sizeof(crcStr), "%08X", crc);

	std::string result = dirPrefix;
	if (!fixedTitle.empty())
	{
		result += " " + fixedTitle;
	}
	result += " (";
	result += crcStr;
	result += ")";

	return result;
}
