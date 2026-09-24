// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "cli/PS2McCommandLine.h"
#include "common/Config.h"
#include "common/version.h"
#include "common/BuildVersion.h"
#include "common/Logger.h"
#include "ui/QtMain.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace fs = std::filesystem;

static int appMain(int argc, char* argv[])
{
	bool cliMode = false;
	for (int i = 1; i < argc; ++i)
	{
		if (std::strcmp(argv[i], "--help") == 0 ||
			std::strcmp(argv[i], "-h") == 0 ||
			std::strcmp(argv[i], "--version") == 0 ||
			std::strcmp(argv[i], "-i") == 0 ||
			std::strcmp(argv[i], "--ignore-ecc") == 0 ||
			std::strcmp(argv[i], "-e") == 0 ||
			std::strcmp(argv[i], "--no-ecc") == 0)
		{
			cliMode = true;
			break;
		}
		if (argv[i][0] != '-')
		{
			std::string arg = argv[i];
			if (arg == "format" || arg == "import" || arg == "export" ||
				arg == "ls" || arg == "dir" || arg == "add" ||
				arg == "extract" || arg == "delete" || arg == "remove" ||
				arg == "mkdir" || arg == "check" || arg == "df" ||
				arg == "clear" || arg == "set")
			{
				cliMode = true;
				break;
			}
			if (arg.find('.') != std::string::npos ||
				arg.find('/') != std::string::npos ||
				arg.find('\\') != std::string::npos ||
				arg.find(':') != std::string::npos)
			{
				cliMode = true;
				break;
			}
		}
	}

	if (cliMode)
	{
#if defined(_WIN32)
		if (AttachConsole(ATTACH_PARENT_PROCESS))
		{
			FILE* stream;
			freopen_s(&stream, "CONOUT$", "w", stdout);
			freopen_s(&stream, "CONOUT$", "w", stderr);
			freopen_s(&stream, "CONIN$", "r", stdin);
		}
#endif
		PS2McCommandLine cli;
		return cli.execute(argc, argv);
	}

	Logger::info("Main: myMCpp version {} ({}, hash {}, date {})",
		myMCpp_VERSION_STRING,
		BuildVersion::GitRev,
		BuildVersion::GitHash,
		BuildVersion::GitDate);

	Config config;
	fs::path config_path;

	QCoreApplication::setApplicationName("myMCpp");
#if defined(__APPLE__)
	const QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
	const QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#endif
	if (!configDir.isEmpty())
	{
		config_path = fs::path(configDir.toStdString()) / "myMCpp.ini";
	}
	else
	{
		config_path = "myMCpp.ini";
	}

	fs::path log_path = config_path.parent_path() / "myMCpp.log";
	Logger::init(log_path.string());

	Logger::info("Main: Using config path: {}", fs::absolute(config_path).string());
	Logger::info("Main: Log file path: {}", fs::absolute(log_path).string());

	if (!config.initialize(config_path))
	{
		Logger::info("Main: Failed to load config, using defaults");
	}

	return runQtMainApp(argc, argv, config);
}

#if defined(_WIN32)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	return appMain(__argc, __argv);
}
#else
int main(int argc, char* argv[])
{
	return appMain(argc, argv);
}
#endif
