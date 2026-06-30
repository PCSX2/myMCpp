// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "FilesSettingsWidget.h"
#include "SettingsWindow.h"
#include "ui_FilesSettingsWidget.h"
#include "Config.h"
#include "QtUtils.h"
#include <QFileDialog>

FilesSettingsWidget::FilesSettingsWidget(SettingsWindow* dialog, QWidget* parent)
	: SettingsWidget(dialog, parent)
	, ui(new Ui::FilesSettingsWidget)
{
	m_rootWidget = new QWidget(this);
	ui->setupUi(m_rootWidget);

	registerHelp(ui->memoryCardPathEdit, tr("Memory Card Directory"), tr("The default directory where memory card images are stored. If not set, the home directory is used."));
	registerHelp(ui->browseButton, tr("Browse Directory"), tr("Open a file dialog to select the memory card directory."));
	registerHelp(ui->importExportPathEdit, tr("Import/Export Directory"), tr("The default directory for importing and exporting save files. If not set, the home directory is used."));
	registerHelp(ui->browseImportExportButton, tr("Browse Directory"), tr("Open a file dialog to select the import/export directory."));

	connect(ui->browseButton, &QPushButton::clicked, this, &FilesSettingsWidget::onBrowseMemoryCardPath);
	connect(ui->browseImportExportButton, &QPushButton::clicked, this, &FilesSettingsWidget::onBrowseImportExportPath);
	connect(ui->memoryCardPathEdit, &QLineEdit::textChanged, this, &SettingsWidget::settingChanged);
	connect(ui->importExportPathEdit, &QLineEdit::textChanged, this, &SettingsWidget::settingChanged);

	addTab(tr("Files"), m_rootWidget);
	loadSettings();
}

FilesSettingsWidget::~FilesSettingsWidget() = default;

void FilesSettingsWidget::changeEvent(QEvent* event)
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

void FilesSettingsWidget::loadSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	ui->memoryCardPathEdit->setText(QString::fromStdString(config->getMemoryCardFolder()));
	ui->importExportPathEdit->setText(QString::fromStdString(config->getImportExportFolder()));
}

void FilesSettingsWidget::saveSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	config->setMemoryCardFolder(ui->memoryCardPathEdit->text().toStdString());
	config->setImportExportFolder(ui->importExportPathEdit->text().toStdString());
}

void FilesSettingsWidget::restoreDefaults()
{
	ui->memoryCardPathEdit->clear();
	ui->importExportPathEdit->clear();
}

void FilesSettingsWidget::onBrowseImportExportPath()
{
	const QString startDir = QtUtils::resolveConfigFolderPath(ui->importExportPathEdit->text());
	QString dir = QFileDialog::getExistingDirectory(this,
		tr("Select Import/Export Folder"), startDir);
	if (!dir.isEmpty())
	{
		ui->importExportPathEdit->setText(dir);
	}
}

void FilesSettingsWidget::onBrowseMemoryCardPath()
{
	const QString startDir = QtUtils::resolveConfigFolderPath(ui->memoryCardPathEdit->text());
	QString dir = QFileDialog::getExistingDirectory(this,
		tr("Select Memory Card Folder"), startDir);
	if (!dir.isEmpty())
	{
		ui->memoryCardPathEdit->setText(dir);
	}
}
