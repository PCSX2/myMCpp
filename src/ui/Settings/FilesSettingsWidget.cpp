// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "FilesSettingsWidget.h"
#include "SettingsWindow.h"
#include "ui_FilesSettingsWidget.h"
#include "Config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>

FilesSettingsWidget::FilesSettingsWidget(SettingsWindow* dialog, QWidget* parent)
	: SettingsWidget(dialog, parent)
	, ui(new Ui::FilesSettingsWidget)
{
	QWidget* container = new QWidget(this);
	ui->setupUi(container);

	registerHelp(ui->memoryCardPathEdit, tr("Memory Card Directory"), tr("The default directory where memory card images are stored."));
	registerHelp(ui->browseButton, tr("Browse Directory"), tr("Open a file dialog to select the memory card directory."));

	connect(ui->browseButton, &QPushButton::clicked, this, &FilesSettingsWidget::onBrowseMemoryCardPath);
	connect(ui->memoryCardPathEdit, &QLineEdit::textChanged, this, &SettingsWidget::settingChanged);

	addTab(tr("Files"), container);
	loadSettings();
}

FilesSettingsWidget::~FilesSettingsWidget() = default;

void FilesSettingsWidget::loadSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config) return;

	ui->memoryCardPathEdit->setText(QString::fromStdString(config->getMemoryCardFolder()));
}

void FilesSettingsWidget::saveSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config) return;

	config->setMemoryCardFolder(ui->memoryCardPathEdit->text().toStdString());
}

void FilesSettingsWidget::restoreDefaults()
{
}

void FilesSettingsWidget::onBrowseMemoryCardPath()
{
	QString dir = QFileDialog::getExistingDirectory(this,
		tr("Select Memory Card Folder"), ui->memoryCardPathEdit->text());
	if (!dir.isEmpty())
	{
		ui->memoryCardPathEdit->setText(dir);
	}
}
