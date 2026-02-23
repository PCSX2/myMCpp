// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <spdlog/spdlog.h>
#include <string>

class Logger
{
public:
	static void init(const std::string& logPath = "");

	template <typename... Args>
	static void trace(spdlog::format_string_t<Args...> fmt, Args&&... args)
	{
		spdlog::trace(fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void debug(spdlog::format_string_t<Args...> fmt, Args&&... args)
	{
		spdlog::debug(fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void info(spdlog::format_string_t<Args...> fmt, Args&&... args)
	{
		spdlog::info(fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
	{
		spdlog::warn(fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void error(spdlog::format_string_t<Args...> fmt, Args&&... args)
	{
		spdlog::error(fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void critical(spdlog::format_string_t<Args...> fmt, Args&&... args)
	{
		spdlog::critical(fmt, std::forward<Args>(args)...);
	}
};
