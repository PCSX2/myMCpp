// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QObject>
#include <QTranslator>
#include <QString>
#include <memory>
#include <string>

class QApplication;
class Config;

class TranslationManager : public QObject
{
	Q_OBJECT

public:
	static TranslationManager& instance();

	void init(QApplication* app, Config* config);
	void loadLanguage(const std::string& lang);

signals:
	void languageChanged();

private:
	TranslationManager();
	~TranslationManager() = default;

	QApplication* m_app = nullptr;
	Config* m_config = nullptr;
	std::unique_ptr<QTranslator> m_translator;
};
