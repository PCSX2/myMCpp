// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "Logger.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>

void Logger::init(const std::string& logPath)
{
	std::vector<spdlog::sink_ptr> sinks;

	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	console_sink->set_pattern("%^[%T] %n: %v%$");
	sinks.push_back(console_sink);

	if (!logPath.empty())
	{
		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath, true);
		file_sink->set_pattern("[%Y-%m-%d %T.%e] [%l] %v");
		sinks.push_back(file_sink);
	}

	auto logger = std::make_shared<spdlog::logger>("myMCpp", begin(sinks), end(sinks));
	logger->set_level(spdlog::level::trace);

	spdlog::register_logger(logger);
	spdlog::set_default_logger(logger);
	spdlog::set_level(spdlog::level::trace);
}
