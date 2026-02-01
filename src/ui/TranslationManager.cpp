// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "TranslationManager.h"
#include "Config.h"
#include "Logger.h"

#include <QApplication>
#include <QDir>

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

    if (m_translator)
    {
        m_app->removeTranslator(m_translator.get());
        m_translator.reset();
    }

    QString langStr = QString::fromStdString(lang);
    Logger::info("Loading language: {}", lang);

    m_translator = std::make_unique<QTranslator>();
    bool loaded = false;
    
    QString externalPath = QString::fromStdString((m_config->getResourcesPath() / "translations").string());
    if (m_translator->load("myMCpp_" + langStr, externalPath))
    {
        loaded = true;
        Logger::info("Loaded translation from external folder: {}", externalPath.toStdString());
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

    if (loaded)
    {
        m_app->installTranslator(m_translator.get());
        Logger::info("Translator installed successfully");
        QEvent* event = new QEvent(QEvent::LanguageChange);
        QApplication::postEvent(m_app, event);
        emit languageChanged();
    }
}

std::vector<std::pair<QString, QString>> TranslationManager::getAvailableLanguages()
{
	std::vector<std::pair<QString, QString>> languages;
	languages.push_back({ "English (US)", "en" });

	QDir dir(":/translations");
	QStringList filters;
	filters << "myMCpp_*.qm";
	dir.setNameFilters(filters);

	for (const QString& filename : dir.entryList())
	{
		QString code = filename.mid(7, filename.length() - 10);
		if (code == "en") continue;

		QLocale locale(code);
		QString name = QLocale::languageToString(locale.language());
		if (locale.territory() != QLocale::AnyCountry)
		{
			name += " (" + QLocale::territoryToString(locale.territory()) + ")";
		}
		
		if (name.isEmpty() || name == "C") name = code;
		languages.push_back({ name, code });
	}

	return languages;
}
