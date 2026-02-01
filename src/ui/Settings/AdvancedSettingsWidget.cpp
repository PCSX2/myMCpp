// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "AdvancedSettingsWidget.h"
#include "SettingsWindow.h"
#include "ui_AdvancedSettingsWidget.h"
#include "Config.h"

AdvancedSettingsWidget::AdvancedSettingsWidget(SettingsWindow* dialog, QWidget* parent)
	: SettingsWidget(dialog, parent)
	, ui(new Ui::AdvancedSettingsWidget)
{
	QWidget* container = new QWidget(this);
	ui->setupUi(container);

	registerHelp(ui->enableDebugLogCheck, tr("Debug Logging"), tr("Enable verbose logging to standard output for debugging purposes."));

	connect(ui->enableDebugLogCheck, &QCheckBox::toggled, this, &SettingsWidget::settingChanged);

	addTab(tr("Advanced"), container);
	loadSettings();
}

AdvancedSettingsWidget::~AdvancedSettingsWidget() = default;

void AdvancedSettingsWidget::loadSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config) return;

	ui->enableDebugLogCheck->setChecked(config->getDebugLogging());
}

void AdvancedSettingsWidget::saveSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config) return;

	config->setDebugLogging(ui->enableDebugLogCheck->isChecked());
}

void AdvancedSettingsWidget::restoreDefaults()
{
	ui->enableDebugLogCheck->setChecked(false);
}
