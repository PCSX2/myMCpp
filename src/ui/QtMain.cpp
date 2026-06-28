// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "QtMain.h"

#include "Config.h"
#include "MainWindow.h"
#include "TranslationManager.h"
#include "ResourcePath.h"
#include "version.h"
#include "Logger.h"

#include <QCoreApplication>
#include <QLibraryInfo>

#if defined(__APPLE__)
#include "CocoaTools.h"
#endif

namespace fs = std::filesystem;

static void initResourcePath(Config& config)
{
#if defined(__APPLE__)
	if (auto bundlePath = CocoaTools::GetResourcePath())
		config.setResourcesPath(*bundlePath);
	else
		config.setResourcesPath("resources");
#else
	config.setResourcesPath(fs::path(QCoreApplication::applicationDirPath().toStdString()) / "resources");
#endif
	ResourcePath::set(config.getResourcesPath());
	Logger::info("Main: Resources path: {}", ResourcePath::get().string());
}

int runQtMainApp(int argc, char* argv[], Config& config)
{
	QApplication app(argc, argv);
	initResourcePath(config);
	app.setApplicationName(MYMCpp_APP_NAME);
	app.setApplicationVersion(myMCpp_VERSION_STRING);
	app.setOrganizationName("myMCpp");

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
