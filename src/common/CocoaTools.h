// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <string>
#include <optional>
#include <cstdint>

struct WindowInfo;

/// Helper functions for things that need Objective-C
namespace CocoaTools
{
	bool CreateMetalLayer(WindowInfo* wi);
	void DestroyMetalLayer(WindowInfo* wi);
	void SetDrawableSize(WindowInfo* wi, uint32_t width, uint32_t height);
	std::optional<float> GetViewRefreshRate(const WindowInfo& wi);

	// Helpers for extracting WindowInfo from a native window handle
	void GetWindowInfoFromWindow(WindowInfo* wi, void* window);

	// Resource path helper
	std::optional<std::string> GetResourcePath();
} // namespace CocoaTools
