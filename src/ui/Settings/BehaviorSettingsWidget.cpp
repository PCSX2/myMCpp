// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "BehaviorSettingsWidget.h"
#include "SettingsWindow.h"
#include "ui_BehaviorSettingsWidget.h"
#include "Config.h"

BehaviorSettingsWidget::BehaviorSettingsWidget(SettingsWindow* dialog, QWidget* parent)
	: SettingsWidget(dialog, parent)
	, ui(new Ui::BehaviorSettingsWidget)
{
	QWidget* container = new QWidget(this);
	ui->setupUi(container);

	registerHelp(ui->warnOnDeleteCheck, tr("Warn Before Deleting"), tr("Show a warning dialog when attempting to delete files from a memory card."));
	registerHelp(ui->hideToTrayCheck, tr("Hide to System Tray"), tr("Minimize the application to the system tray instead of the taskbar."));

	connect(ui->warnOnDeleteCheck, &QCheckBox::toggled, this, &SettingsWidget::settingChanged);
	connect(ui->hideToTrayCheck, &QCheckBox::toggled, this, &SettingsWidget::settingChanged);

	addTab(tr("Behavior"), container);
	loadSettings();
}

BehaviorSettingsWidget::~BehaviorSettingsWidget() = default;

void BehaviorSettingsWidget::loadSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config) return;

	ui->warnOnDeleteCheck->setChecked(config->getWarnOnDelete());
	ui->hideToTrayCheck->setChecked(config->getHideToTrayOnClose());
}

void BehaviorSettingsWidget::saveSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config) return;

	config->setWarnOnDelete(ui->warnOnDeleteCheck->isChecked());
	config->setHideToTrayOnClose(ui->hideToTrayCheck->isChecked());
}

void BehaviorSettingsWidget::restoreDefaults()
{
	ui->warnOnDeleteCheck->setChecked(true);
	ui->hideToTrayCheck->setChecked(false);
}
