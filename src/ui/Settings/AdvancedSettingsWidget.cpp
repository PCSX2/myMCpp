// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "AdvancedSettingsWidget.h"
#include "SettingsWindow.h"
#include "ui_AdvancedSettingsWidget.h"
#include "Config.h"
#include <QFileDialog>
#include <QMessageBox>

AdvancedSettingsWidget::AdvancedSettingsWidget(SettingsWindow* dialog, QWidget* parent)
	: SettingsWidget(dialog, parent)
	, ui(new Ui::AdvancedSettingsWidget)
{
	QWidget* container = new QWidget(this);
	ui->setupUi(container);

	registerHelp(ui->enableDebugLogCheck, tr("Debug Logging"), tr("Enable verbose logging to standard output for debugging purposes."));
	registerHelp(ui->exportSettingsButton, tr("Export Settings"), tr("Save the current settings to a JSON file for backup or sharing."));
	registerHelp(ui->importSettingsButton, tr("Import Settings"), tr("Load settings from a previously exported JSON file."));

	connect(ui->enableDebugLogCheck, &QCheckBox::toggled, this, &SettingsWidget::settingChanged);
	connect(ui->exportSettingsButton, &QPushButton::clicked, this, &AdvancedSettingsWidget::onExportSettings);
	connect(ui->importSettingsButton, &QPushButton::clicked, this, &AdvancedSettingsWidget::onImportSettings);

	addTab(tr("Advanced"), container);
	loadSettings();
}

AdvancedSettingsWidget::~AdvancedSettingsWidget() = default;

void AdvancedSettingsWidget::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		if (ui)
		{
			ui->retranslateUi(this);
		}
	}
	SettingsWidget::changeEvent(event);
}

void AdvancedSettingsWidget::loadSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	ui->enableDebugLogCheck->setChecked(config->getDebugLogging());
}

void AdvancedSettingsWidget::saveSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	config->setDebugLogging(ui->enableDebugLogCheck->isChecked());
}

void AdvancedSettingsWidget::restoreDefaults()
{
	ui->enableDebugLogCheck->setChecked(false);
}

void AdvancedSettingsWidget::onExportSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	QString fileName = QFileDialog::getSaveFileName(this,
		tr("Export Settings"),
		"myMCpp-settings.json",
		tr("JSON Files (*.json)"));

	if (!fileName.isEmpty())
	{
		if (config->saveAs(fileName.toStdString()))
		{
			QMessageBox::information(this, tr("Export Successful"),
				tr("Settings have been exported to %1").arg(fileName));
		}
		else
		{
			QMessageBox::warning(this, tr("Export Failed"),
				tr("Failed to export settings to %1").arg(fileName));
		}
	}
}

void AdvancedSettingsWidget::onImportSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Import Settings"),
		QString(),
		tr("JSON Files (*.json)"));

	if (!fileName.isEmpty())
	{
		QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Confirm Import"),
			tr("This will overwrite your current settings. Are you sure you want to continue?"),
			QMessageBox::Yes | QMessageBox::No);

		if (reply == QMessageBox::Yes)
		{
			if (config->load(fileName.toStdString()))
			{
				loadSettings();
				emit settingChanged();
				QMessageBox::information(this, tr("Import Successful"),
					tr("Settings have been imported from %1").arg(fileName));
			}
			else
			{
				QMessageBox::warning(this, tr("Import Failed"),
					tr("Failed to import settings from %1").arg(fileName));
			}
		}
	}
}
