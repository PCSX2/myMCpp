// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class ResourcePath
{
public:
	static void set(const fs::path& path) { s_path = path; }
	static const fs::path& get() { return s_path; }
	static fs::path shaders() { return s_path / "shaders"; }
	static fs::path icons() { return s_path / "icons"; }

private:
	static inline fs::path s_path = "resources";
};
