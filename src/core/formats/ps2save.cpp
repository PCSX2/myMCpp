// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+
// PS2 save container handling (MAX/EMS/SharkPort/CodeBreaker/PSV) follows the mymc++ / mymc implementations and PS2 save format documentation.

#include "ps2save.h"
#include "Logger.h"
#include "round.h"
#include "lzari.h"
#include "sjis.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <zlib.h>

namespace
{
	inline bool modeIsDir(uint16_t mode) { return (mode & DF_DIR) != 0; }
	inline bool modeIsFile(uint16_t mode) { return (mode & DF_FILE) != 0; }
} // namespace

class PS2SaveFile::Impl
{
public:
	Error& m_error;
	std::vector<PS2SaveEntry> entries;
	std::string title;
	SaveFormat format;

	explicit Impl(Error& error)
		: m_error(error)
		, format(SaveFormat::UNKNOWN)
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
	: pImpl(std::make_unique<Impl>(m_error))
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

	pImpl->format = detectFormat(filename);

	switch (pImpl->format)
	{
		case SaveFormat::MAX_DRIVE:
			return pImpl->loadMaxDrive(file);
		case SaveFormat::EMS:
			return pImpl->loadEms(file);
		case SaveFormat::SHARKPORT:
		case SaveFormat::XPORT: // X-Port uses same format as SharkPort
			return pImpl->loadSharkPort(file);
		case SaveFormat::CODEBREAKER:
			return pImpl->loadCodeBreaker(file);
		case SaveFormat::PSV:
			return pImpl->loadPsv(file);
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
		format = pImpl->format;
	}

	switch (format)
	{
		case SaveFormat::MAX_DRIVE:
			return pImpl->saveMaxDrive(file);
		case SaveFormat::EMS:
			return pImpl->saveEms(file);
		default:
			return m_error.Fail("Unsupported save format for writing");
	}
}

std::vector<PS2SaveEntry>& PS2SaveFile::getEntries()
{
	return pImpl->entries;
}

const std::vector<PS2SaveEntry>& PS2SaveFile::getEntries() const
{
	return pImpl->entries;
}

std::string PS2SaveFile::getTitle() const
{
	return pImpl->title;
}

void PS2SaveFile::setTitle(const std::string& title)
{
	pImpl->title = title;
}

SaveFormat PS2SaveFile::getFormat() const
{
	return pImpl->format;
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
			std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
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
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

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
		return m_error.Fail("Corrupt MAX header");
	struct Hdr
	{
		char magic[12];
		uint32_t crc;
		char dirname[32];
		char iconsys[32];
		uint32_t clen;
		uint32_t dirlen;
		uint32_t length;
	};
	const Hdr* h = reinterpret_cast<const Hdr*>(hdr);
	if (std::memcmp(h->magic, PS2SAVE_MAX_MAGIC, 12) != 0)
	{
		return m_error.Fail("Not a MAX Drive save");
	}
	std::string dirname = zeroTerminate(std::string(h->dirname, 32));
	uint32_t clen = h->clen;
	uint32_t dirlen = h->dirlen;
	uint32_t length = h->length;

	std::vector<uint8_t> compressed;
	if (clen == length)
	{
		file.seekg(0, std::ios::end);
		size_t remaining = static_cast<size_t>(file.tellg()) - 0x5C;
		file.seekg(0x5C, std::ios::beg);
		if (!readFixed(file, remaining, compressed))
			return false;
	}
	else
	{
		if (!readFixed(file, clen - 4, compressed)) // -4 because crc already read
			return false;
	}

	auto decompressed = lzariDecompress(compressed, length);
	if (decompressed.size() != length)
		return m_error.Fail("MAX decompress size mismatch");

	entries.clear();
	entries.reserve(dirlen);
	size_t off = 0;
	auto timestamp = time(nullptr);
	for (uint32_t i = 0; i < dirlen; ++i)
	{
		if (off + 36 > decompressed.size())
			return m_error.Fail("MAX directory truncated");
		uint32_t l = *reinterpret_cast<const uint32_t*>(&decompressed[off]);
		std::string name(reinterpret_cast<const char*>(&decompressed[off + 4]), 32);
		name = zeroTerminate(name);
		off += 36;
		if (off + l > decompressed.size())
			return m_error.Fail("MAX file truncated");
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
		entries.push_back({ent, std::move(data)});
	}
	title = dirname;
	format = SaveFormat::MAX_DRIVE;
	return true;
}

bool PS2SaveFile::Impl::loadSharkPort(std::ifstream& file)
{
	const std::string fmtName = (format == SaveFormat::XPORT) ? "X-Port" : "SharkPort";

	std::vector<uint8_t> magic;
	if (!readFixed(file, 17, magic))
		return false;
	if (std::memcmp(magic.data(), PS2SAVE_SPS_MAGIC, 13) != 0)
	{
		return m_error.Fail("Not a " + fmtName + " save");
	}

	uint32_t savetype = 0;
	if (!readBytes(file, &savetype, 4))
		return false;

	uint32_t dirname_len = 0;
	if (!readBytes(file, &dirname_len, 4))
		return false;
	std::vector<uint8_t> dirname_data;
	if (!readFixed(file, dirname_len, dirname_data))
		return false;
	std::string dirname(dirname_data.begin(), dirname_data.end());

	uint32_t datestamp_len = 0;
	if (!readBytes(file, &datestamp_len, 4))
		return false;
	std::vector<uint8_t> unused;
	if (!readFixed(file, datestamp_len, unused))
		return false;

	uint32_t comment_len = 0;
	if (!readBytes(file, &comment_len, 4))
		return false;
	if (!readFixed(file, comment_len, unused))
		return false;

	uint32_t flen = 0;
	if (!readBytes(file, &flen, 4))
		return false;

	uint16_t hlen = 0;
	if (!readBytes(file, &hlen, 2))
		return false;

	std::vector<uint8_t> name_buf(64);
	if (!readBytes(file, name_buf.data(), 64))
		return false;

	uint32_t dirlen = 0;
	if (!readBytes(file, &dirlen, 4))
		return false;

	if (!readFixed(file, 8, unused)) // skip 8 bytes
		return false;

	uint16_t dirmode = 0;
	if (!readBytes(file, &dirmode, 2))
		return false;

	if (!readFixed(file, 2, unused)) // skip 2 bytes
		return false;

	std::vector<uint8_t> created_data;
	if (!readFixed(file, 8, created_data))
		return false;

	std::vector<uint8_t> modified_data;
	if (!readFixed(file, 8, modified_data))
		return false;

	if (hlen > 98)
	{
		if (!readFixed(file, hlen - 98, unused))
			return false;
	}

	dirmode = ((dirmode / 256) % 256) + ((dirmode % 256) * 256);

	if (!(dirmode & DF_DIR) || dirlen < 2)
	{
		return m_error.Fail("Invalid " + fmtName + " directory header");
	}

	entries.clear();
	entries.reserve(dirlen - 2);

	for (uint32_t i = 0; i < dirlen - 2; ++i)
	{
		uint16_t fhlen = 0;
		if (!readBytes(file, &fhlen, 2))
			return false;

		std::vector<uint8_t> fname_buf(64);
		if (!readBytes(file, fname_buf.data(), 64))
			return false;
		std::string fname(reinterpret_cast<const char*>(fname_buf.data()));
		fname = zeroTerminate(fname);

		uint32_t fsize = 0;
		if (!readBytes(file, &fsize, 4))
			return false;
		if (!readFixed(file, 8, unused)) // skip 8 bytes
			return false;

		uint16_t fmode = 0;
		if (!readBytes(file, &fmode, 2))
			return false;
		if (!readFixed(file, 2, unused)) // skip 2 bytes
			return false;

		std::vector<uint8_t> fcreated_data;
		if (!readFixed(file, 8, fcreated_data))
			return false;
		PS2McTod fcreated = unpackTod(fcreated_data);

		std::vector<uint8_t> fmodified_data;
		if (!readFixed(file, 8, fmodified_data))
			return false;
		PS2McTod fmodified = unpackTod(fmodified_data);

		if (fhlen > 98)
		{
			if (!readFixed(file, fhlen - 98, unused))
				return false;
		}

		fmode = ((fmode / 256) % 256) + ((fmode % 256) * 256);

		if (!(fmode & DF_FILE))
		{
			return m_error.Fail("Non-file in " + fmtName + " directory");
		}

		std::vector<uint8_t> fdata;
		if (!readFixed(file, fsize, fdata))
			return false;

		PS2McDirEntry ent;
		ent.mode = fmode | DF_EXISTS;
		ent.length = fsize;
		ent.name = fname;
		ent.created = fcreated;
		ent.modified = fmodified;

		entries.push_back({ent, std::move(fdata)});
	}

	title = dirname;
	if (format != SaveFormat::XPORT)
	{
		format = SaveFormat::SHARKPORT;
	}
	return true;
}

namespace
{
	// clang-format off
	static const uint8_t PS2SAVE_CBS_RC4S[] = {
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

	std::vector<uint8_t> rc4_crypt(const std::vector<uint8_t>& s, const std::vector<uint8_t>& data)
	{
		std::vector<uint8_t> s_copy = s;
		std::vector<uint8_t> result = data;

		size_t i = 0, j = 0;
		for (size_t n = 0; n < data.size(); ++n)
		{
			i = (i + 1) % 256;
			j = (j + s_copy[i]) % 256;
			std::swap(s_copy[i], s_copy[j]);
			uint8_t k = s_copy[(s_copy[i] + s_copy[j]) % 256];
			result[n] ^= k;
		}
		return result;
	}
} // namespace

bool PS2SaveFile::Impl::loadCodeBreaker(std::ifstream& file)
{
	std::vector<uint8_t> magic;
	if (!readFixed(file, 4, magic))
		return false;
	if (std::memcmp(magic.data(), PS2SAVE_CBS_MAGIC, 4) != 0)
	{
		return m_error.Fail("Not a CodeBreaker save");
	}

	uint32_t d04 = 0, hlen = 0;
	if (!readBytes(file, &d04, 4) || !readBytes(file, &hlen, 4))
		return false;

	if (hlen < 124)
		return m_error.Fail("CodeBreaker header too short");

	std::vector<uint8_t> header;
	if (!readFixed(file, hlen - 12, header))
		return false;

	uint32_t dlen = 0, flen = 0;
	std::memcpy(&dlen, header.data() + 0, 4);
	std::memcpy(&flen, header.data() + 4, 4);

	char dirname[32];
	std::memcpy(dirname, header.data() + 8, 32);

	std::vector<uint8_t> created_data(header.begin() + 40, header.begin() + 48);
	PS2McTod created = unpackTod(created_data);

	std::vector<uint8_t> modified_data(header.begin() + 48, header.begin() + 56);
	PS2McTod modified = unpackTod(modified_data);

	uint32_t dirmode = 0;
	std::memcpy(&dirmode, header.data() + 64, 4);

	if (!(dirmode & DF_DIR))
	{
		dirmode = DF_DIR | DF_RWX | DF_0400;
	}

	if (todToTime(created) == 0)
		created = timeToTod(std::time(nullptr));
	if (todToTime(modified) == 0)
		modified = timeToTod(std::time(nullptr));

	size_t remaining = 0;
	{
		auto cur = file.tellg();
		file.seekg(0, std::ios::end);
		remaining = static_cast<size_t>(file.tellg() - cur);
		file.seekg(cur, std::ios::beg);
	}

	size_t body_len = flen;
	if (body_len > remaining)
	{
		body_len = remaining;
	}
	if (body_len != flen && body_len != flen - hlen)
	{
		return m_error.Fail("Unexpected EOF");
	}

	std::vector<uint8_t> body;
	if (!readFixed(file, body_len, body))
		return false;

	std::vector<uint8_t> rc4_key(std::begin(PS2SAVE_CBS_RC4S), std::end(PS2SAVE_CBS_RC4S));
	auto decrypted = rc4_crypt(rc4_key, body);

	std::vector<uint8_t> decompressed(dlen);
	z_stream stream{};
	stream.avail_in = static_cast<uInt>(decrypted.size());
	stream.next_in = decrypted.data();
	stream.avail_out = static_cast<uInt>(dlen);
	stream.next_out = decompressed.data();

	if (inflateInit(&stream) != Z_OK || inflate(&stream, Z_FINISH) != Z_STREAM_END)
	{
		return m_error.Fail("CodeBreaker zlib decompress failed");
	}
	inflateEnd(&stream);

	entries.clear();
	size_t off = 0;

	while (off < decompressed.size())
	{
		if (off + 64 > decompressed.size())
			break;

		uint32_t fsize = 0;
		std::memcpy(&fsize, decompressed.data() + off + 16, 4);
		uint16_t fmode = 0;
		std::memcpy(&fmode, decompressed.data() + off + 20, 2);

		std::vector<uint8_t> fcreated_data(decompressed.begin() + off + 0,
			decompressed.begin() + off + 8);
		std::vector<uint8_t> fmodified_data(decompressed.begin() + off + 8,
			decompressed.begin() + off + 16);

		char fname[32];
		std::memcpy(fname, decompressed.data() + off + 32, 32);

		off += 64;
		if (off + fsize > decompressed.size())
			break;

		PS2McTod fcreated = unpackTod(fcreated_data);
		PS2McTod fmodified = unpackTod(fmodified_data);

		if (!(fmode & DF_FILE))
		{
			return m_error.Fail("Non-file in CodeBreaker save");
		}

		std::vector<uint8_t> fdata(decompressed.begin() + off, decompressed.begin() + off + fsize);
		off += fsize;

		PS2McDirEntry ent;
		ent.mode = fmode | DF_EXISTS;
		ent.length = fsize;
		ent.name = zeroTerminate(std::string(fname, 32));
		ent.created = fcreated;
		ent.modified = fmodified;

		entries.push_back({ent, std::move(fdata)});
	}

	title = zeroTerminate(std::string(dirname, 32));
	format = SaveFormat::CODEBREAKER;
	return true;
}

bool PS2SaveFile::Impl::loadPsv(std::ifstream& file)
{
	std::vector<uint8_t> magic_data;
	if (!readFixed(file, 4, magic_data))
		return false;
	if (std::memcmp(magic_data.data(), PS2SAVE_PSV_MAGIC, 4) != 0)
	{
		return m_error.Fail("Not a PSV file");
	}

	uint32_t version = 0, next_section_size = 0, savetype = 0;
	if (!readBytes(file, &version, 4))
		return false;
	std::vector<uint8_t> unused;
	if (!readFixed(file, 40, unused)) // signature
		return false;
	if (!readFixed(file, 8, unused)) // reserved
		return false;
	if (!readBytes(file, &next_section_size, 4))
		return false;
	if (!readBytes(file, &savetype, 4))
		return false;

	if (version != 0)
	{
		return m_error.Fail("Unsupported PSV version");
	}

	if (savetype != 2 && savetype != 1)
	{
		return m_error.Fail("Unsupported PSV save type");
	}

	if (savetype == 2)
	{
		uint32_t unused_field, sys_pos, sys_size;
		uint32_t icon1_pos, icon1_size, icon2_pos, icon2_size;
		uint32_t icon3_pos, icon3_size, files_count;

		if (!readBytes(file, &unused_field, 4) ||
			!readBytes(file, &sys_pos, 4) ||
			!readBytes(file, &sys_size, 4) ||
			!readBytes(file, &icon1_pos, 4) ||
			!readBytes(file, &icon1_size, 4) ||
			!readBytes(file, &icon2_pos, 4) ||
			!readBytes(file, &icon2_size, 4) ||
			!readBytes(file, &icon3_pos, 4) ||
			!readBytes(file, &icon3_size, 4) ||
			!readBytes(file, &files_count, 4))
		{
			return false;
		}

		std::vector<uint8_t> root_created_data;
		if (!readFixed(file, 8, root_created_data))
			return false;
		std::vector<uint8_t> root_modified_data;
		if (!readFixed(file, 8, root_modified_data))
			return false;
		uint32_t root_size = 0;
		uint32_t root_mode = 0;
		if (!readBytes(file, &root_size, 4) || !readBytes(file, &root_mode, 4))
			return false;

		std::vector<uint8_t> root_filename(32);
		if (!readBytes(file, root_filename.data(), 32))
			return false;

		if (!(root_mode & DF_DIR))
		{
			return m_error.Fail("PSV root is not a directory");
		}

		title = zeroTerminate(std::string(reinterpret_cast<const char*>(root_filename.data()), 32));

		entries.clear();
		entries.reserve(files_count);

		std::vector<std::pair<uint32_t, PS2McDirEntry>> file_list;
		for (uint32_t i = 0; i < files_count; ++i)
		{
			std::vector<uint8_t> file_created_data;
			if (!readFixed(file, 8, file_created_data))
				return false;
			std::vector<uint8_t> file_modified_data;
			if (!readFixed(file, 8, file_modified_data))
				return false;
			uint32_t file_size = 0;
			uint32_t file_mode = 0;
			if (!readBytes(file, &file_size, 4) || !readBytes(file, &file_mode, 4))
				return false;

			std::vector<uint8_t> filename_buf(32);
			if (!readBytes(file, filename_buf.data(), 32))
				return false;

			uint32_t file_offset = 0;
			if (!readBytes(file, &file_offset, 4))
				return false;

			PS2McDirEntry ent;
			ent.mode = static_cast<uint16_t>(file_mode | DF_EXISTS);
			ent.length = file_size;
			ent.name = zeroTerminate(std::string(reinterpret_cast<const char*>(filename_buf.data()), 32));
			ent.created = unpackTod(file_created_data);
			ent.modified = unpackTod(file_modified_data);

			if (file_mode & DF_FILE)
			{
				file_list.push_back({file_offset, ent});
			}
		}

		for (const auto& [offset, ent] : file_list)
		{
			file.seekg(offset);
			std::vector<uint8_t> fdata;
			if (!readFixed(file, ent.length, fdata))
				return false;
			entries.push_back({ent, std::move(fdata)});
		}
	}
	else
	{
		uint32_t save_size = 0, save_offset = 0;
		if (!readBytes(file, &save_size, 4) || !readBytes(file, &save_offset, 4))
			return false;
		if (!readFixed(file, 20, unused)) // reserved
			return false;
		uint32_t unused_next = 0;
		if (!readBytes(file, &unused_next, 4))
			return false;
		if (!readFixed(file, 4, unused)) // reserved
			return false;

		std::vector<uint8_t> prod_code(20);
		if (!readBytes(file, prod_code.data(), 20))
			return false;

		title = zeroTerminate(std::string(reinterpret_cast<const char*>(prod_code.data()), 20));

		entries.clear();
		uint16_t mode = DF_RWX | DF_FILE | DF_0080 | DF_0400 | DF_EXISTS;

		PS2McDirEntry ent;
		ent.mode = mode;
		ent.length = save_size;
		ent.name = "SAVEGAME.PSX";
		ent.created = ent.modified = timeToTod(std::time(nullptr));

		file.seekg(save_offset);
		std::vector<uint8_t> fdata;
		if (!readFixed(file, save_size, fdata))
			return false;
		entries.push_back({ent, std::move(fdata)});
	}

	format = SaveFormat::PSV;
	return true;
}

bool PS2SaveFile::Impl::saveMaxDrive(std::ofstream& file)
{
	std::vector<uint8_t> blob;
	const bool hasDirHeader = !entries.empty() && (entries[0].dirEntry.mode & DF_DIR);
	const size_t firstFileIdx = hasDirHeader ? 1 : 0;

	for (size_t i = firstFileIdx; i < entries.size(); ++i)
	{
		const auto& e = entries[i];
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
	if (!entries.empty())
	{
		std::string dName = hasDirHeader ? entries[0].dirEntry.name : title;
		if (dName.empty())
		{
			dName = entries[0].dirEntry.name;
		}
		const size_t copyLen = std::min(dName.size(), sizeof(dirname) - 1);
		std::memcpy(dirname, dName.c_str(), copyLen);
		dirname[copyLen] = '\0';
	}
	char iconsys[32] = {0};
	uint32_t clen = static_cast<uint32_t>(compressed.size()) + 4; // include CRC field itself
	uint32_t dirlen = static_cast<uint32_t>(hasDirHeader ? entries.size() - 1 : entries.size());
	uint32_t crc = 0; // omitted
	file.write(PS2SAVE_MAX_MAGIC, 12);
	file.write(reinterpret_cast<const char*>(&crc), 4);
	file.write(dirname, 32);
	file.write(iconsys, 32);
	file.write(reinterpret_cast<const char*>(&clen), 4);
	file.write(reinterpret_cast<const char*>(&dirlen), 4);
	file.write(reinterpret_cast<const char*>(&length), 4);
	file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
	file.flush();
	if (!file)
		return m_error.Fail("Failed to write MAX save");
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
	entries.clear();
	entries.reserve(static_cast<size_t>(fileCount) + 1);
	PS2SaveEntry dirSlot;
	dirSlot.dirEntry = dirent;
	dirSlot.data.clear();
	entries.push_back(std::move(dirSlot));
	for (uint32_t i = 0; i < fileCount; ++i)
	{
		std::vector<uint8_t> entRaw;
		if (!readFixed(file, PS2MC_DIRENT_LENGTH, entRaw))
			return false;
		auto ent = unpackDirEntry(entRaw);
		if (!modeIsFile(ent.mode))
		{
			const std::string name = ent.name.empty() ? std::string("<unnamed>") : ent.name;
			Logger::warn("Skipping subdir (entry {}): {}", i, name);
			continue;
		}
		std::vector<uint8_t> data;
		if (!readFixed(file, ent.length, data))
			return false;
		size_t pad = roundUp(static_cast<int>(ent.length), 1024) - ent.length;
		if (pad)
		{
			std::vector<uint8_t> padData;
			if (!readFixed(file, pad, padData))
				return false;
		}
		entries.push_back({ent, std::move(data)});
	}
	format = SaveFormat::EMS;
	title = dirent.name;
	return true;
}

bool PS2SaveFile::Impl::saveEms(std::ofstream& file)
{
	const bool hasDirHeader = !entries.empty() && (entries[0].dirEntry.mode & DF_DIR);
	const size_t numFiles = hasDirHeader ? entries.size() - 1 : entries.size();

	PS2McDirEntry root{};
	if (hasDirHeader)
	{
		root = entries[0].dirEntry;
	}
	else
	{
		root.name = entries.empty() ? "SAVE" : entries[0].dirEntry.name;
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

	for (size_t idx = hasDirHeader ? 1 : 0; idx < entries.size(); ++idx)
	{
		const auto& e = entries[idx];
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
		return m_error.Fail("Failed to write PSU save");
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
					crc = (crc >> 1) ^ 0xEDB88320;
				else
					crc = crc >> 1;
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
		cleanTitle.pop_back();

	auto fixChar = [](char c) -> char {
		unsigned char uc = static_cast<unsigned char>(c);
		if (uc < 32 || uc >= 127)
			return '_';
		if (c == '<' || c == '>')
			return '(';
		if (c == ':' || c == '"' || c == '/' || c == '\\' || c == '|')
			return '_';
		if (c == '?' || c == '*')
			return '_';
		return c;
	};

	std::string fixedTitle;
	for (char c : cleanTitle)
		fixedTitle += fixChar(c);

	char crcStr[16];
	std::snprintf(crcStr, sizeof(crcStr), "%08X", crc);

	std::string result = dirPrefix;
	if (!fixedTitle.empty())
		result += " " + fixedTitle;
	result += " (";
	result += crcStr;
	result += ")";

	return result;
}
