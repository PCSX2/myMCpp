// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "SettingsWindow.h"
#include "Config.h"
#include "Logger.h"
#include "Themes.h"
#include "TranslationManager.h"
#include "ui_SettingsWindow.h"

#include "GeneralSettingsWidget.h"
#include "GraphicsSettingsWidget.h"
#include "FilesSettingsWidget.h"
#include "AdvancedSettingsWidget.h"

#include <QTimer>
#include <QEvent>

SettingsWindow::SettingsWindow(Config* config, QWidget* parent)
	: QDialog(parent)
	, m_config(config)
	, ui(new Ui::SettingsWindow)
{
	ui->setupUi(this);

	while (ui->settingsContainer->count() > 0)
		ui->settingsContainer->removeWidget(ui->settingsContainer->widget(0));
	ui->settingsCategory->clear();

	addSettingsWidget(new GeneralSettingsWidget(this), tr("General"),
		QIcon::fromTheme("settings-fill"),
		tr("<strong>General Settings</strong><hr>Configure language, theme, and application behavior."));

	addSettingsWidget(new GraphicsSettingsWidget(this), tr("Graphics"),
		QIcon::fromTheme("computer-line"),
		tr("<strong>Graphics Settings</strong><hr>Configure rendering options, camera, lighting, and visual effects."));

	addSettingsWidget(new FilesSettingsWidget(this), tr("Files"),
		QIcon::fromTheme("folder-open-line"),
		tr("<strong>File Settings</strong><hr>Manage default paths for memory cards, import/export settings, and recent files."));

	addSettingsWidget(new AdvancedSettingsWidget(this), tr("Advanced"),
		QIcon::fromTheme("equalizer-line"),
		tr("<strong>Advanced Settings</strong><hr>Advanced options for window behavior, debugging, and developer tools."));

	ui->settingsCategory->setCurrentRow(0);
	updateDescription(0);

	connect(ui->settingsCategory, &QListWidget::currentRowChanged, this, &SettingsWindow::onCategorySelected);
	connect(ui->restoreDefaultsButton, &QPushButton::clicked, this, &SettingsWindow::onRestoreDefaults);
	connect(ui->closeButton, &QPushButton::clicked, this, &SettingsWindow::onClose);

	m_saveTimer = new QTimer(this);
	m_saveTimer->setSingleShot(true);
	m_saveTimer->setInterval(500);
	connect(m_saveTimer, &QTimer::timeout, this, &SettingsWindow::saveToDisk);

	connect(&TranslationManager::instance(), &TranslationManager::languageChanged,
		this, &SettingsWindow::languageChanged);
}

SettingsWindow::~SettingsWindow()
{
}

void SettingsWindow::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		if (ui)
		{
			ui->retranslateUi(this);
			updateDescription(ui->settingsCategory->currentRow());
		}
	}
	QDialog::changeEvent(event);
}

void SettingsWindow::onSettingChanged()
{
	for (auto* widget : m_settingsWidgets)
	{
		widget->saveSettings();
	}
	if (m_config)
	{
		Themes::UpdateApplicationTheme(m_config);
		TranslationManager::instance().loadLanguage(m_config->getLanguage());
	}
	emit applicationSettingsChanged();
	m_saveTimer->start();
}

void SettingsWindow::saveToDisk()
{
	if (m_config)
	{
		m_config->save();
	}
}

void SettingsWindow::addSettingsWidget(SettingsWidget* widget, const QString& name, const QIcon& icon, const QString& description)
{
	m_settingsWidgets.push_back(widget);
	m_descriptions.push_back(description);

	new QListWidgetItem(icon, name, ui->settingsCategory);
	ui->settingsContainer->addWidget(widget);

	connect(widget, &SettingsWidget::settingChanged, this, &SettingsWindow::onSettingChanged);
}

void SettingsWindow::onCategorySelected(int row)
{
	if (row >= 0 && row < ui->settingsContainer->count())
	{
		ui->settingsContainer->setCurrentIndex(row);
		updateDescription(row);
	}
}

void SettingsWindow::updateDescription(int index)
{
	if (index >= 0 && index < static_cast<int>(m_descriptions.size()))
	{
		m_currentCategoryDescription = m_descriptions[index];
		ui->helpText->setHtml(m_descriptions[index]);
	}
}

void SettingsWindow::registerWidgetHelp(QObject* widget, const QString& title, const QString& description)
{
	if (!widget)
		return;

	m_helpMap[widget] = {title, description};
	widget->installEventFilter(this);
}

bool SettingsWindow::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::Enter)
	{
		auto it = m_helpMap.find(watched);
		if (it != m_helpMap.end())
		{
			QString helpHtml = QString("<strong>%1</strong><hr>%2").arg(it->title, it->description);
			ui->helpText->setHtml(helpHtml);
		}
	}
	else if (event->type() == QEvent::Leave)
	{
		if (m_helpMap.contains(watched))
		{
			ui->helpText->setHtml(m_currentCategoryDescription);
		}
	}

	return QDialog::eventFilter(watched, event);
}

void SettingsWindow::onClose()
{
	for (auto* widget : m_settingsWidgets)
	{
		widget->saveSettings();
	}

	if (m_config)
	{
		m_config->save();
	}

	accept();
}

void SettingsWindow::onRestoreDefaults()
{
	int currentRow = ui->settingsCategory->currentRow();
	if (currentRow >= 0 && currentRow < static_cast<int>(m_settingsWidgets.size()))
	{
		m_settingsWidgets[currentRow]->restoreDefaults();
		onSettingChanged();
	}
}
