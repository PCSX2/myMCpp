// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MainWindow.h"
#include "ps2mc_cli.h"
#include "Config.h"
#include <QApplication>
#if !defined(__APPLE__)
#include <QVulkanInstance>
#endif
#include <filesystem>
#include "Logger.h"
#include "ResourcePath.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <QTranslator>
#include <QLibraryInfo>
#include "TranslationManager.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

#if defined(__APPLE__)
#include "CocoaTools.h"
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

	QApplication app(argc, argv);

	app.setApplicationName("myMCpp");
	app.setApplicationVersion("1.0.0");
	app.setOrganizationName("myMCpp");

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

	Logger::info("[main] Using config path: {}", fs::absolute(config_path).string());
	Logger::info("[main] Log file path: {}", fs::absolute(log_path).string());

	if (!config.initialize(config_path))
	{
		qWarning("Failed to load config, using defaults");
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
	config.setResourcesPath((fs::path(QCoreApplication::applicationDirPath().toStdString()) / "resources").string());
#endif
	ResourcePath::set(config.getResourcesPath());
	Logger::info("[main] Resources path: {}", ResourcePath::get().string());

#if !defined(__APPLE__)
	QVulkanInstance vulkanInstance;
#if defined(QT_DEBUG)
	vulkanInstance.setLayers({"VK_LAYER_KHRONOS_validation"});
#endif
	if (!vulkanInstance.create())
	{
		qWarning("Failed to create Vulkan instance");
	}
#endif

	// Load translations
	TranslationManager::instance().init(&app, &config);
	TranslationManager::instance().loadLanguage(config.getLanguage());

	QTranslator qtTranslator;
	if (qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
	{
		app.installTranslator(&qtTranslator);
	}


	MainWindow window(&config);
	window.show();

	return app.exec();
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
