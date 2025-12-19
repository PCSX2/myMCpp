// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "ps2iconsys.h"
#include "sjis.h"
#include <cstring>
#include <stdexcept>

namespace
{

	uint32_t readBEu32(const uint8_t* ptr)
	{
		return (static_cast<uint32_t>(ptr[0]) << 24) |
		       (static_cast<uint32_t>(ptr[1]) << 16) |
		       (static_cast<uint32_t>(ptr[2]) << 8) |
		       (static_cast<uint32_t>(ptr[3]));
	}

	float readBEf32(const uint8_t* ptr)
	{
		uint32_t v = readBEu32(ptr);
		float f;
		std::memcpy(&f, &v, sizeof(float));
		return f;
	}

} // namespace

struct IconSysHeader
{
	uint16_t unknown1;
	uint16_t unknown2;
	uint32_t animCount;
	uint32_t animSpeed;
	uint32_t animLoopCount;
	uint32_t unknown3;
	// Followed by animation speed entries (animCount * 2 bytes each)
	// Then title entries
	// Then icon data
};

class PS2IconSys::Impl
{
public:
	// Data fields
	std::vector<uint8_t> data;
	int iconCount = 0;
	int animCount = 0;
	std::string title;
	std::string subtitle;
	std::string iconFileNormal;
	std::string iconFileCopy;
	std::string iconFileDelete;
	uint16_t titleLineOffset = 0;

	float bgTransparency = 0.0f;
	std::array<IconSysColor, 4> bgColors;
	std::array<IconSysLight, 3> lightDirs;
	std::array<IconSysColor, 3> lightColors;
	IconSysColor ambientLight;

	void parse()
	{
		if (data.size() < 964)
		{
			return;
		}

		if (memcmp(data.data(), "PS2D", 4) != 0)
		{
			return;
		}

		const uint8_t* ptr = data.data();

		auto extractString = [](const uint8_t* src, size_t maxLen) -> std::string {
			std::string result;
			for (size_t i = 0; i < maxLen && src[i] != 0; ++i)
			{
				result += static_cast<char>(src[i]);
			}
			return result;
		};

		titleLineOffset = static_cast<uint16_t>((ptr[0x06] << 8) | ptr[0x07]);
		bgTransparency = static_cast<float>(ptr[0x0C]) / 128.0f;

		for (int i = 0; i < 4; ++i)
		{
			size_t offset = 0x10 + (i * 16);
			// Each color channel is stored as a uint32 (only low byte used), 0x00-0x80 range
			bgColors[i].r = static_cast<float>(ptr[offset + 0]) / 128.0f;
			bgColors[i].g = static_cast<float>(ptr[offset + 4]) / 128.0f;
			bgColors[i].b = static_cast<float>(ptr[offset + 8]) / 128.0f;
			bgColors[i].a = static_cast<float>(ptr[offset + 12]) / 128.0f;
		}

		for (int i = 0; i < 3; ++i)
		{
			size_t offset = 0x50 + (i * 16);
			lightDirs[i].x = readBEf32(ptr + offset + 0);
			lightDirs[i].y = readBEf32(ptr + offset + 4);
			lightDirs[i].z = readBEf32(ptr + offset + 8);
			lightDirs[i].w = readBEf32(ptr + offset + 12);
		}

		for (int i = 0; i < 3; ++i)
		{
			size_t offset = 0x80 + (i * 16);
			lightColors[i].r = static_cast<float>(ptr[offset + 0]) / 128.0f;
			lightColors[i].g = static_cast<float>(ptr[offset + 4]) / 128.0f;
			lightColors[i].b = static_cast<float>(ptr[offset + 8]) / 128.0f;
			lightColors[i].a = static_cast<float>(ptr[offset + 12]) / 128.0f;
		}

		ambientLight.r = static_cast<float>(ptr[0xB0 + 0]) / 128.0f;
		ambientLight.g = static_cast<float>(ptr[0xB0 + 4]) / 128.0f;
		ambientLight.b = static_cast<float>(ptr[0xB0 + 8]) / 128.0f;
		ambientLight.a = static_cast<float>(ptr[0xB0 + 12]) / 128.0f;

		std::string titleSjis;
		for (size_t i = 0; i < 68 && ptr[0xC0 + i] != 0; ++i)
		{
			titleSjis += static_cast<char>(ptr[0xC0 + i]);
		}

		if (titleLineOffset > 0 && titleLineOffset < titleSjis.length())
		{
			std::string titleSjis1 = titleSjis.substr(0, titleLineOffset);
			std::string titleSjis2 = titleSjis.substr(titleLineOffset);
			title = ShiftJIS::toUtf8(titleSjis1);
			subtitle = ShiftJIS::toUtf8(titleSjis2);
		}
		else
		{
			title = ShiftJIS::toUtf8(titleSjis);
			subtitle = "";
		}

		iconFileNormal = extractString(ptr + 0x104, 64);
		iconFileCopy = extractString(ptr + 0x144, 64);
		iconFileDelete = extractString(ptr + 0x184, 64);

		iconCount = !iconFileNormal.empty() ? 1 : 0;
		animCount = 0;
	}
};

PS2IconSys::PS2IconSys()
	: pImpl(std::make_unique<Impl>())
{
}
PS2IconSys::~PS2IconSys() = default;

void PS2IconSys::load(const std::vector<uint8_t>& data)
{
	pImpl->data = data;
	pImpl->parse();
}

std::string PS2IconSys::getTitle(const std::string& encoding) const
{
	(void)encoding;
	return pImpl->title;
}

std::string PS2IconSys::getSubtitle(const std::string& encoding) const
{
	(void)encoding;
	return pImpl->subtitle;
}

std::string PS2IconSys::getIconFileNormal() const
{
	return pImpl->iconFileNormal;
}

std::string PS2IconSys::getIconFileCopy() const
{
	return pImpl->iconFileCopy;
}

std::string PS2IconSys::getIconFileDelete() const
{
	return pImpl->iconFileDelete;
}

std::vector<uint8_t> PS2IconSys::getIconData(int index) const
{
	(void)index;
	return std::vector<uint8_t>();
}

int PS2IconSys::getIconCount() const
{
	return pImpl->iconCount;
}

int PS2IconSys::getAnimationCount() const
{
	return pImpl->animCount;
}

int PS2IconSys::getAnimationSpeed(int index) const
{
	(void)index;
	return 0;
}

float PS2IconSys::getBgTransparency() const
{
	return pImpl->bgTransparency;
}

const std::array<IconSysColor, 4>& PS2IconSys::getBgColors() const
{
	return pImpl->bgColors;
}

const std::array<IconSysLight, 3>& PS2IconSys::getLightDirections() const
{
	return pImpl->lightDirs;
}

const std::array<IconSysColor, 3>& PS2IconSys::getLightColors() const
{
	return pImpl->lightColors;
}

const IconSysColor& PS2IconSys::getAmbientLight() const
{
	return pImpl->ambientLight;
}
