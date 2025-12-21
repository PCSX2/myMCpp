// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "Themes.h"
#include "Config.h"
#include "Logger.h"
#include "ResourcePath.h"

#include <QtCore/QFile>
#include <QtGui/QPalette>
#include <QtGui/QPixmapCache>
#include <QtGui/QStyleHints>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleFactory>

namespace Themes
{
	static void SetStyleFromSettings(Config* config);
	static void SetIconThemeFromStyle();
	static void SetColorScheme(Qt::ColorScheme color_scheme);
}

static QString s_unthemed_style_name;
static QPalette s_unthemed_palette;
static bool s_unthemed_style_name_set = false;

static Qt::ColorScheme s_color_scheme = Qt::ColorScheme::Unknown;

const char* Themes::GetDefaultThemeName()
{
#ifdef __APPLE__
	return "";
#else
	return "dark";
#endif
}

void Themes::UpdateApplicationTheme(Config* config)
{
	if (!s_unthemed_style_name_set)
	{
		s_unthemed_style_name_set = true;
		s_unthemed_style_name = QApplication::style()->objectName();
		s_unthemed_palette = QApplication::palette();
	}

	SetStyleFromSettings(config);
	SetIconThemeFromStyle();

	QPixmapCache::clear();
}

bool Themes::IsDarkApplicationTheme()
{
	if (s_color_scheme != Qt::ColorScheme::Unknown)
		return s_color_scheme == Qt::ColorScheme::Dark;

	QPalette palette = qApp->palette();
	return palette.windowText().color().value() > palette.window().color().value();
}

void Themes::SetIconThemeFromStyle()
{
	// Add resource icon path to search paths
	QString iconsPath = QString::fromStdString(ResourcePath::icons().string());
	QStringList paths = QIcon::themeSearchPaths();
	if (!paths.contains(iconsPath)) {
		paths.prepend(iconsPath);
		QIcon::setThemeSearchPaths(paths);
	}

	const bool dark = IsDarkApplicationTheme();
	QIcon::setThemeName(dark ? QStringLiteral("white") : QStringLiteral("black"));
}

void Themes::SetStyleFromSettings(Config* config)
{
	std::string theme = config->getTheme();
	if (theme.empty()) theme = GetDefaultThemeName();

	if (theme == "light")
	{
		qApp->setStyle(QStyleFactory::create("Fusion"));
		qApp->setPalette(s_unthemed_palette);
		qApp->setStyleSheet(QString());
		SetColorScheme(Qt::ColorScheme::Light);
	}
#ifdef _WIN32
	else if (theme == "windowsvista")
	{
		qApp->setStyle(QStyleFactory::create("windowsvista"));
		qApp->setPalette(s_unthemed_palette);
		qApp->setStyleSheet(QString());

		// We can't set this to Qt::ColorScheme::Light because that breaks high
		// contrast themes on Windows.
		SetColorScheme(Qt::ColorScheme::Unknown);
	}
#endif
	else if (theme == "dark")
	{
		qApp->setStyle(QStyleFactory::create("Fusion"));

		// Standard Dark Fusion Palette
		QColor darkGray(53, 53, 53);
		QColor gray(128, 128, 128);
		QColor black(25, 25, 25);
		QColor blue(42, 130, 218);

		QPalette darkPalette;
		darkPalette.setColor(QPalette::Window, darkGray);
		darkPalette.setColor(QPalette::WindowText, Qt::white);
		darkPalette.setColor(QPalette::Base, black);
		darkPalette.setColor(QPalette::AlternateBase, darkGray);
		darkPalette.setColor(QPalette::ToolTipBase, black);
		darkPalette.setColor(QPalette::ToolTipText, Qt::white);
		darkPalette.setColor(QPalette::Text, Qt::white);
		darkPalette.setColor(QPalette::Button, darkGray);
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::Link, blue);
		darkPalette.setColor(QPalette::Highlight, blue);
		darkPalette.setColor(QPalette::HighlightedText, Qt::white);

		darkPalette.setColor(QPalette::Active, QPalette::Button, darkGray.lighter());
		darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
		darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

		qApp->setPalette(darkPalette);
		qApp->setStyleSheet(QString());
		SetColorScheme(Qt::ColorScheme::Dark);
	}
	else if (theme == "none")
	{
		qApp->setStyle(s_unthemed_style_name);
		qApp->setPalette(s_unthemed_palette);
		qApp->setStyleSheet(QString());
		SetColorScheme(Qt::ColorScheme::Unknown);
	}
	else
	{
		// Fallback / System
		qApp->setStyle(s_unthemed_style_name);
		qApp->setPalette(s_unthemed_palette);
		qApp->setStyleSheet(QString());
		SetColorScheme(Qt::ColorScheme::Unknown);
	}
}

static void Themes::SetColorScheme(Qt::ColorScheme color_scheme)
{
	s_color_scheme = color_scheme;
	
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	qApp->styleHints()->setColorScheme(color_scheme);
#endif
}
