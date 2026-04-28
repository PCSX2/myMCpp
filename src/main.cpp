// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "ps2mc_cli.h"
#include "Config.h"
#include "version.h"
#include "BuildVersion.h"
#include "QtMain.h"
#include <filesystem>
#include "Logger.h"
#include "ResourcePath.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>

#if defined(_WIN32)
#include <Windows.h>
#endif

#if defined(__APPLE__)
#include "CocoaTools.h"
#endif

namespace fs = std::filesystem;

#if !defined(__APPLE__)
static fs::path getExecutableDir(const char* argv0)
{
#if defined(_WIN32)
	char modulePath[MAX_PATH] = {};
	const DWORD length = GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
	if (length > 0)
	{
		return fs::path(modulePath).parent_path();
	}
#endif
	if (argv0 != nullptr && argv0[0] != '\0')
	{
		std::error_code ec;
		fs::path resolved = fs::weakly_canonical(fs::path(argv0), ec);
		if (!ec && !resolved.empty())
		{
			if (resolved.has_parent_path())
				return resolved.parent_path();
			return resolved;
		}
	}

	std::error_code ec;
	fs::path current = fs::current_path(ec);
	return ec ? fs::path(".") : current;
}
#endif

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

#if defined(_WIN32)
	char* appdata = nullptr;
	size_t len = 0;
	if (_dupenv_s(&appdata, &len, "APPDATA") == 0 && appdata != nullptr)
	{
		config_path = fs::path(appdata) / "myMCpp" / "config.json";
		free(appdata);
	}
	else
	{
		config_path = "config.json";
	}
#else
	const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
	if (xdg_config && xdg_config[0] != '\0')
	{
		config_path = fs::path(xdg_config) / "myMCpp" / "config.json";
	}
	else
	{
		const char* home = std::getenv("HOME");
		if (home && home[0] != '\0')
		{
			config_path = fs::path(home) / ".config" / "myMCpp" / "config.json";
		}
		else
		{
			config_path = "config.json";
		}
	}
#endif

	fs::path log_path = config_path.parent_path() / "myMCpp.log";
	Logger::init(log_path.string());

	Logger::info("Main: Using config path: {}", fs::absolute(config_path).string());
	Logger::info("Main: Log file path: {}", fs::absolute(log_path).string());

	if (!config.initialize(config_path))
	{
		Logger::info("Main: Failed to load config, using defaults");
	}

#if defined(__APPLE__)
	if (auto bundlePath = CocoaTools::GetResourcePath())
	{
		config.setResourcesPath(*bundlePath);
	}
	else
	{
		config.setResourcesPath("resources");
	}
#else
	config.setResourcesPath((getExecutableDir(argc > 0 ? argv[0] : nullptr) / "resources").string());
#endif
	ResourcePath::set(config.getResourcesPath());
	Logger::info("Main: Resources path: {}", ResourcePath::get().string());

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
