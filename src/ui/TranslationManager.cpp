// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "TranslationManager.h"
#include "Config.h"
#include "Logger.h"

#include <QApplication>
#include <QDir>
#include <QSet>
#include <QWidget>

TranslationManager& TranslationManager::instance()
{
	static TranslationManager instance;
	return instance;
}

TranslationManager::TranslationManager()
	: m_translator(std::make_unique<QTranslator>())
{
}

void TranslationManager::init(QApplication* app, Config* config)
{
	m_app = app;
	m_config = config;
}

void TranslationManager::loadLanguage(const std::string& lang)
{
	if (!m_app || !m_config)
	{
		Logger::error("TranslationManager not initialized!");
		return;
	}

	if (lang == m_currentLanguage)
		return;

	if (m_translator)
	{
		m_app->removeTranslator(m_translator.get());
		m_translator.reset();
	}

	QString langStr = QString::fromStdString(lang);
	Logger::info("Loading language: {}", lang);

	m_translator = std::make_unique<QTranslator>();
	bool loaded = false;

	// Don't need to translate English
	if (langStr != QStringLiteral("en"))
	{
		QString appTranslations = QApplication::applicationDirPath() + QStringLiteral("/translations");
		if (m_translator->load("myMCpp_" + langStr, appTranslations))
		{
			loaded = true;
			Logger::info("Loaded translation from app translations folder: {}", appTranslations.toStdString());
		}
		else if (m_translator->load("myMCpp_" + langStr,
					 QString::fromStdString((m_config->getResourcesPath() / "translations").string())))
		{
			loaded = true;
			Logger::info("Loaded translation from external resources folder");
		}
		// Try resource path
		else if (m_translator->load(":/translations/myMCpp_" + langStr + ".qm"))
		{
			loaded = true;
			Logger::info("Loaded translation from resource");
		}
		else
		{
			Logger::warn("Failed to load translation for language: {}", lang);
		}
	}

	m_currentLanguage = lang;

	if (loaded)
	{
		m_app->installTranslator(m_translator.get());
		Logger::info("Translator installed successfully");
	}

	emit languageChanged();
}

std::vector<std::pair<QString, QString>> TranslationManager::getAvailableLanguages()
{
	std::vector<std::pair<QString, QString>> languages;
	languages.push_back({QStringLiteral("English (US)"), QStringLiteral("en")});

	QSet<QString> seenCodes;
	seenCodes.insert(QStringLiteral("en"));

	auto addLanguagesFromDir = [&](const QDir& dir) {
		if (!dir.exists())
			return;

		const QStringList files = dir.entryList(QStringList(QStringLiteral("myMCpp_*.qm")), QDir::Files);
		for (const QString& filename : files)
		{
			const QString code = filename.mid(7, filename.length() - 10);
			if (code.isEmpty() || seenCodes.contains(code))
				continue;

			QLocale locale(code);
			QString name = QLocale::languageToString(locale.language());
			if (locale.territory() != QLocale::AnyCountry)
			{
				name += QStringLiteral(" (") + QLocale::territoryToString(locale.territory()) + QStringLiteral(")");
			}

			if (name.isEmpty() || name == QStringLiteral("C"))
				name = code;

			seenCodes.insert(code);
			languages.emplace_back(name, code);
		}
	};

	addLanguagesFromDir(QDir(QStringLiteral(":/translations")));
	addLanguagesFromDir(QDir(QApplication::applicationDirPath() + QStringLiteral("/translations")));

	if (m_config)
	{
		const auto translationsPath = (m_config->getResourcesPath() / "translations").string();
		addLanguagesFromDir(QDir(QString::fromStdString(translationsPath)));
	}

	return languages;
}
