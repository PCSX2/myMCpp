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
    }

    QString langStr = QString::fromStdString(lang);
    Logger::info("Loading language: {}", lang);

    bool loaded = false;
    
    QString externalPath = QString::fromStdString((m_config->getResourcesPath() / "translations").string());
    if (m_translator->load("myMCpp_" + langStr, externalPath))
    {
        loaded = true;
        Logger::info("Loaded translation from external folder");
    }
    // Try resource path
    else if (m_translator->load(":/translations/myMCpp_" + langStr))
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
        emit languageChanged();
    }
}
