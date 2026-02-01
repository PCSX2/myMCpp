// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <array>

// PS2 icon.sys structure
struct IconSysColor
{
	float r, g, b, a;
};

struct IconSysLight
{
	float x, y, z, w;
};

class PS2IconSys
{
public:
	PS2IconSys();
	~PS2IconSys();

	void load(const std::vector<uint8_t>& data);

	std::string getTitle(const std::string& encoding = "") const;
	std::string getSubtitle(const std::string& encoding = "") const;

	std::string getIconFileNormal() const;
	std::string getIconFileCopy() const;
	std::string getIconFileDelete() const;

	std::vector<uint8_t> getIconData(int index) const;

	int getIconCount() const;
	int getAnimationCount() const;
	int getAnimationSpeed(int index) const;

	float getBgTransparency() const;
	const std::array<IconSysColor, 4>& getBgColors() const;
	const std::array<IconSysLight, 3>& getLightDirections() const; // 3 lights, 4 floats each (coords)
	const std::array<IconSysColor, 3>& getLightColors() const; // 3 lights, 4 floats each (RGBA)
	const IconSysColor& getAmbientLight() const; // 1 ambient, 4 floats (RGBA)

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};
