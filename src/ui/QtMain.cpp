// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "QtMain.h"

#include "Config.h"
#include "MainWindow.h"
#include "TranslationManager.h"
#include "version.h"
#include "Logger.h"

#include <QLibraryInfo>

int runQtMainApp(int argc, char* argv[], Config& config)
{
	QApplication app(argc, argv);
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
