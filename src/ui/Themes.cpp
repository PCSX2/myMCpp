// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "Themes.h"
#include "Config.h"

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
} // namespace Themes

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
	const bool dark = IsDarkApplicationTheme();
	QIcon::setThemeName(dark ? QStringLiteral("white") : QStringLiteral("black"));
}

void Themes::SetStyleFromSettings(Config* config)
{
	std::string theme = config->getTheme();
	if (theme.empty())
		theme = GetDefaultThemeName();

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
	else if (theme == "pizzabrown")
	{
		// Custom palette by KamFretoZ, a Pizza Tower Reference!
		// With a mixtures of Light Brown, Peachy/Creamy White, Latte-like Color.
		// Thanks to Jordan for the idea :P
		// Alternative light theme.
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor gray(128, 128, 128);
		const QColor extr(248, 192, 88);
		const QColor main(233, 187, 147);
		const QColor comp(248, 230, 213);
		const QColor highlight(188, 100, 60);

		QPalette pizzaPalette;
		pizzaPalette.setColor(QPalette::Window, main);
		pizzaPalette.setColor(QPalette::WindowText, Qt::black);
		pizzaPalette.setColor(QPalette::Base, comp);
		pizzaPalette.setColor(QPalette::AlternateBase, extr);
		pizzaPalette.setColor(QPalette::ToolTipBase, comp);
		pizzaPalette.setColor(QPalette::ToolTipText, Qt::black);
		pizzaPalette.setColor(QPalette::Text, Qt::black);
		pizzaPalette.setColor(QPalette::Button, extr);
		pizzaPalette.setColor(QPalette::ButtonText, Qt::black);
		pizzaPalette.setColor(QPalette::Link, highlight.darker());
		pizzaPalette.setColor(QPalette::Highlight, highlight);
		pizzaPalette.setColor(QPalette::HighlightedText, Qt::black);

		pizzaPalette.setColor(QPalette::Active, QPalette::Button, extr);
		pizzaPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray.darker());
		pizzaPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray.darker());
		pizzaPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::gray);
		pizzaPalette.setColor(QPalette::Disabled, QPalette::Light, gray.lighter());

		qApp->setPalette(pizzaPalette);
		qApp->setStyleSheet(QString());
		SetColorScheme(Qt::ColorScheme::Light);
	}
	else if (theme == "greymatter")
	{
		// Custom palette by KamFretoZ, A sleek and stylish gray
		// that are meant to be easy on the eyes as the main color.
		// Alternative dark theme.
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor darkGray(46, 52, 64);
		const QColor lighterGray(59, 66, 82);
		const QColor gray(111, 111, 111);
		const QColor blue(198, 238, 255);

		QPalette greyMatterPalette;
		greyMatterPalette.setColor(QPalette::Window, darkGray);
		greyMatterPalette.setColor(QPalette::WindowText, Qt::white);
		greyMatterPalette.setColor(QPalette::Base, lighterGray);
		greyMatterPalette.setColor(QPalette::AlternateBase, darkGray);
		greyMatterPalette.setColor(QPalette::ToolTipBase, darkGray);
		greyMatterPalette.setColor(QPalette::ToolTipText, Qt::white);
		greyMatterPalette.setColor(QPalette::Text, Qt::white);
		greyMatterPalette.setColor(QPalette::Button, lighterGray);
		greyMatterPalette.setColor(QPalette::ButtonText, Qt::white);
		greyMatterPalette.setColor(QPalette::Link, blue);
		greyMatterPalette.setColor(QPalette::Highlight, lighterGray.lighter());
		greyMatterPalette.setColor(QPalette::HighlightedText, Qt::white);
		greyMatterPalette.setColor(QPalette::PlaceholderText, QColor(Qt::white).darker());

		greyMatterPalette.setColor(QPalette::Active, QPalette::Button, lighterGray);
		greyMatterPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray.lighter());
		greyMatterPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray.lighter());
		greyMatterPalette.setColor(QPalette::Disabled, QPalette::Text, gray.lighter());
		greyMatterPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

		qApp->setPalette(greyMatterPalette);
		qApp->setStyleSheet(QString());
		SetColorScheme(Qt::ColorScheme::Dark);
	}
	else if (theme == "cobaltsky")
	{
		// Custom palette by KamFretoZ, A soothing deep royal blue
		// that are meant to be easy on the eyes as the main color.
		// Alternative dark theme.
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor gray(150, 150, 150);
		const QColor royalBlue(29, 41, 81);
		const QColor darkishBlue(17, 30, 108);
		const QColor lighterBlue(25, 32, 130);
		const QColor highlight(36, 93, 218);
		const QColor link(0, 202, 255);

		QPalette cobaltSkyPalette;
		cobaltSkyPalette.setColor(QPalette::Window, royalBlue);
		cobaltSkyPalette.setColor(QPalette::WindowText, Qt::white);
		cobaltSkyPalette.setColor(QPalette::Base, royalBlue.lighter());
		cobaltSkyPalette.setColor(QPalette::AlternateBase, darkishBlue);
		cobaltSkyPalette.setColor(QPalette::ToolTipBase, darkishBlue);
		cobaltSkyPalette.setColor(QPalette::ToolTipText, Qt::white);
		cobaltSkyPalette.setColor(QPalette::Text, Qt::white);
		cobaltSkyPalette.setColor(QPalette::Button, lighterBlue);
		cobaltSkyPalette.setColor(QPalette::ButtonText, Qt::white);
		cobaltSkyPalette.setColor(QPalette::Link, link);
		cobaltSkyPalette.setColor(QPalette::Highlight, highlight);
		cobaltSkyPalette.setColor(QPalette::HighlightedText, Qt::white);

		cobaltSkyPalette.setColor(QPalette::Active, QPalette::Button, lighterBlue);
		cobaltSkyPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
		cobaltSkyPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
		cobaltSkyPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
		cobaltSkyPalette.setColor(QPalette::Disabled, QPalette::Light, gray);

		qApp->setPalette(cobaltSkyPalette);
		qApp->setStyleSheet(QString());
		SetColorScheme(Qt::ColorScheme::Dark);
	}
	else if (theme == "amoled")
	{
		// Custom palette by KamFretoZ, A pure concentrated darkness
		// of a theme designed for maximum eye comfort and benefits
		// OLED screens.
		qApp->setStyle(QStyleFactory::create("Fusion"));

		const QColor black(0, 0, 0);
		const QColor gray(25, 25, 25);
		const QColor lighterGray(75, 75, 75);
		const QColor blue(198, 238, 255);

		QPalette AMOLEDPalette;
		AMOLEDPalette.setColor(QPalette::Window, black);
		AMOLEDPalette.setColor(QPalette::WindowText, Qt::white);
		AMOLEDPalette.setColor(QPalette::Base, gray);
		AMOLEDPalette.setColor(QPalette::AlternateBase, black);
		AMOLEDPalette.setColor(QPalette::ToolTipBase, gray);
		AMOLEDPalette.setColor(QPalette::ToolTipText, Qt::white);
		AMOLEDPalette.setColor(QPalette::Text, Qt::white);
		AMOLEDPalette.setColor(QPalette::Button, gray);
		AMOLEDPalette.setColor(QPalette::ButtonText, Qt::white);
		AMOLEDPalette.setColor(QPalette::Link, blue);
		AMOLEDPalette.setColor(QPalette::Highlight, lighterGray);
		AMOLEDPalette.setColor(QPalette::HighlightedText, Qt::white);
		AMOLEDPalette.setColor(QPalette::PlaceholderText, QColor(Qt::white).darker());

		AMOLEDPalette.setColor(QPalette::Active, QPalette::Button, gray);
		AMOLEDPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(Qt::white).darker());
		AMOLEDPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(Qt::white).darker());
		AMOLEDPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(Qt::white).darker());
		AMOLEDPalette.setColor(QPalette::Disabled, QPalette::Light, QColor(Qt::white).darker());

		qApp->setPalette(AMOLEDPalette);
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
