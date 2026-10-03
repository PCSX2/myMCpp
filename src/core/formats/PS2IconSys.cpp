// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+
// icon.sys structure decoding is based on mymc++ / mymc and public docs such as PS2 Save Tools and Martin Åkesson’s PS2 icon format notes.

#include "PS2IconSys.h"
#include "common/sjis.h"
#include <cstring>

class PS2IconSys::Impl
{
public:
	std::vector<uint8_t> m_data;
	bool m_valid = false;
	int m_iconCount = 0;
	int m_animCount = 0;
	std::string m_title;
	std::string m_subtitle;
	std::string m_iconFileIdle;
	std::string m_iconFileCopy;
	std::string m_iconFileDelete;
	uint16_t m_titleLineOffset = 0;

	float m_bgTransparency = 0.0f;
	std::array<IconSysColor, 4> m_bgColors;
	std::array<IconSysLight, 3> m_lightDirs;
	std::array<IconSysColor, 3> m_lightColors;
	IconSysColor m_ambientLight;

	void parse()
	{
		m_valid = false;
		if (m_data.size() < 964)
		{
			return;
		}

		if (std::memcmp(m_data.data(), "PS2D", 4) != 0)
		{
			return;
		}

		const uint8_t* ptr = m_data.data();

		auto extractString = [](const uint8_t* src, size_t maxLen) -> std::string {
			std::string result;
			for (size_t i = 0; i < maxLen && src[i] != 0; ++i)
			{
				result += static_cast<char>(src[i]);
			}
			return result;
		};

		m_titleLineOffset = static_cast<uint16_t>(ptr[0x06] | (ptr[0x07] << 8));
		m_bgTransparency = std::min(1.0f, static_cast<float>(ptr[0x0C]) / 128.0f);

		for (int i = 0; i < 4; ++i)
		{
			size_t offset = 0x10 + (i * 16);
			m_bgColors[i].r = static_cast<float>(ptr[offset + 0]) / 255.0f;
			m_bgColors[i].g = static_cast<float>(ptr[offset + 4]) / 255.0f;
			m_bgColors[i].b = static_cast<float>(ptr[offset + 8]) / 255.0f;
			m_bgColors[i].a = std::min(1.0f, static_cast<float>(ptr[offset + 12]) / 128.0f);
		}

		std::memcpy(m_lightDirs.data(), ptr + 0x50, sizeof(m_lightDirs));
		std::memcpy(m_lightColors.data(), ptr + 0x80, sizeof(m_lightColors));
		std::memcpy(&m_ambientLight, ptr + 0xB0, sizeof(m_ambientLight));

		std::string titleSjis;
		for (size_t i = 0; i < 68 && ptr[0xC0 + i] != 0; ++i)
		{
			titleSjis += static_cast<char>(ptr[0xC0 + i]);
		}

		if (m_titleLineOffset > 0 && m_titleLineOffset < titleSjis.length())
		{
			std::string titleSjis1 = titleSjis.substr(0, m_titleLineOffset);
			std::string titleSjis2 = titleSjis.substr(m_titleLineOffset);
			m_title = ShiftJIS::toUtf8(titleSjis1);
			m_subtitle = ShiftJIS::toUtf8(titleSjis2);
		}
		else
		{
			m_title = ShiftJIS::toUtf8(titleSjis);
			m_subtitle = "";
		}

		m_iconFileIdle = extractString(ptr + 0x104, 64);
		m_iconFileCopy = extractString(ptr + 0x144, 64);
		m_iconFileDelete = extractString(ptr + 0x184, 64);

		m_iconCount = !m_iconFileIdle.empty() ? 1 : 0;
		m_animCount = 0;
		m_valid = true;
	}
};

PS2IconSys::PS2IconSys()
	: m_impl(std::make_unique<Impl>())
{
}

PS2IconSys::~PS2IconSys() = default;

void PS2IconSys::load(const std::vector<uint8_t>& data)
{
	m_impl->m_data = data;
	m_impl->parse();
}

bool PS2IconSys::isValid() const
{
	return m_impl->m_valid;
}

std::string PS2IconSys::getTitle(const std::string& encoding) const
{
	(void)encoding;
	return m_impl->m_title;
}

std::string PS2IconSys::getSubtitle(const std::string& encoding) const
{
	(void)encoding;
	return m_impl->m_subtitle;
}

std::string PS2IconSys::getIconFileIdle() const
{
	return m_impl->m_iconFileIdle;
}

std::string PS2IconSys::getIconFileCopy() const
{
	return m_impl->m_iconFileCopy;
}

std::string PS2IconSys::getIconFileDelete() const
{
	return m_impl->m_iconFileDelete;
}

std::vector<uint8_t> PS2IconSys::getIconData(int index) const
{
	(void)index;
	return std::vector<uint8_t>();
}

int PS2IconSys::getIconCount() const
{
	return m_impl->m_iconCount;
}

int PS2IconSys::getAnimationCount() const
{
	return m_impl->m_animCount;
}

int PS2IconSys::getAnimationSpeed(int index) const
{
	(void)index;
	return 0;
}

float PS2IconSys::getBgTransparency() const
{
	return m_impl->m_bgTransparency;
}

const std::array<IconSysColor, 4>& PS2IconSys::getBgColors() const
{
	return m_impl->m_bgColors;
}

const std::array<IconSysLight, 3>& PS2IconSys::getLightDirections() const
{
	return m_impl->m_lightDirs;
}

const std::array<IconSysColor, 3>& PS2IconSys::getLightColors() const
{
	return m_impl->m_lightColors;
}

const IconSysColor& PS2IconSys::getAmbientLight() const
{
	return m_impl->m_ambientLight;
}
