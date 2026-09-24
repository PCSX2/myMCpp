// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "GeneralSettingsWidget.h"
#include "SettingsWindow.h"
#include "ui_GeneralSettingsWidget.h"

#include "TranslationManager.h"

#include "common/Config.h"

GeneralSettingsWidget::GeneralSettingsWidget(SettingsWindow* dialog, QWidget* parent)
	: SettingsWidget(dialog, parent)
	, ui(new Ui::GeneralSettingsWidget)
{
	m_rootWidget = new QWidget(this);
	ui->setupUi(m_rootWidget);

	registerHelp(ui->languageCombo, tr("Language"), tr("Select the language for the application interface."));
	registerHelp(ui->themeCombo, tr("Theme"), tr("Select the color theme for the application."));
	registerHelp(ui->warnOnDeleteCheck, tr("Warn Before Deleting"), tr("Show a warning dialog when attempting to delete files from a memory card."));
	registerHelp(ui->asciiModeCheck, tr("ASCII Mode"), tr("Use ASCII characters only for filenames when exporting. This helps with compatibility on some systems."));
	registerHelp(ui->forceImportCheck, tr("Force Import"), tr("When importing files, overwrite existing files without prompting."));
	registerHelp(ui->discordRpcCheck, tr("Enable Discord Rich Presence"), tr("Show your myMCpp activity in Discord while the GUI is open."));

	ui->languageCombo->clear();
	auto languages = TranslationManager::instance().getAvailableLanguages();
	for (const auto& lang : languages)
	{
		ui->languageCombo->addItem(lang.first, lang.second);
	}

	ui->themeCombo->clear();
	ui->themeCombo->addItem(tr("None"), "none");
	ui->themeCombo->addItem(tr("Dark (Default)"), "dark");
	ui->themeCombo->addItem(tr("Light"), "light");
	ui->themeCombo->addItem(tr("Pizza Brown [Light]"), "pizzabrown");
	ui->themeCombo->addItem(tr("Grey Matter [Dark]"), "greymatter");
	ui->themeCombo->addItem(tr("Cobalt Sky [Dark]"), "cobaltsky");
	ui->themeCombo->addItem(tr("AMOLED [Black]"), "amoled");
#ifdef _WIN32
	ui->themeCombo->addItem(tr("Windows Vista"), "windowsvista");
#endif

	connect(ui->themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->warnOnDeleteCheck, &QCheckBox::toggled, this, &SettingsWidget::settingChanged);
	connect(ui->asciiModeCheck, &QCheckBox::toggled, this, &SettingsWidget::settingChanged);
	connect(ui->forceImportCheck, &QCheckBox::toggled, this, &SettingsWidget::settingChanged);
	connect(ui->discordRpcCheck, &QCheckBox::toggled, this, &SettingsWidget::settingChanged);

	addTab(tr("General"), m_rootWidget);
	loadSettings();
}

GeneralSettingsWidget::~GeneralSettingsWidget() = default;

void GeneralSettingsWidget::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		if (ui && m_rootWidget)
		{
			ui->retranslateUi(m_rootWidget);
		}
	}
	SettingsWidget::changeEvent(event);
}

void GeneralSettingsWidget::loadSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	int langIdx = ui->languageCombo->findData(QString::fromStdString(config->UI.Language));
	if (langIdx >= 0)
		ui->languageCombo->setCurrentIndex(langIdx);

	QString t = QString::fromStdString(config->UI.Theme).toLower();
	int tIdx = ui->themeCombo->findData(t);
	if (tIdx >= 0)
		ui->themeCombo->setCurrentIndex(tIdx);

	ui->warnOnDeleteCheck->setChecked(config->Behavior.WarnOnDelete);
	ui->asciiModeCheck->setChecked(config->UI.AsciiMode);
	ui->forceImportCheck->setChecked(config->Behavior.ForceImport);
	ui->discordRpcCheck->setChecked(config->Behavior.DiscordRPCEnabled);
}

void GeneralSettingsWidget::saveSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	config->UI.Language = ui->languageCombo->currentData().toString().toStdString();
	config->UI.Theme = ui->themeCombo->currentData().toString().toStdString();
	config->Behavior.WarnOnDelete = ui->warnOnDeleteCheck->isChecked();
	config->UI.AsciiMode = ui->asciiModeCheck->isChecked();
	config->Behavior.ForceImport = ui->forceImportCheck->isChecked();
	config->Behavior.DiscordRPCEnabled = ui->discordRpcCheck->isChecked();
}
