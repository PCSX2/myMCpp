// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <string>

class Config;

namespace Themes
{
	void UpdateApplicationTheme(Config* config);
	bool IsDarkApplicationTheme();
	const char* GetDefaultThemeName();
}
