// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "FilesSettingsWidget.h"
#include "SettingsWindow.h"
#include "ui_FilesSettingsWidget.h"
#include "Config.h"
#include <QFileDialog>
#include <QListWidget>
#include <QMessageBox>

FilesSettingsWidget::FilesSettingsWidget(SettingsWindow* dialog, QWidget* parent)
	: SettingsWidget(dialog, parent)
	, ui(new Ui::FilesSettingsWidget)
{
	m_rootWidget = new QWidget(this);
	ui->setupUi(m_rootWidget);

	registerHelp(ui->memoryCardPathEdit, tr("Memory Card Directory"), tr("The default directory where memory card images are stored."));
	registerHelp(ui->browseButton, tr("Browse Directory"), tr("Open a file dialog to select the memory card directory."));
	registerHelp(ui->importExportPathEdit, tr("Import/Export Directory"), tr("The default directory for importing and exporting save files. If not set, the memory card directory is used."));
	registerHelp(ui->browseImportExportButton, tr("Browse Directory"), tr("Open a file dialog to select the import/export directory."));
	registerHelp(ui->recentFilesList, tr("Recent Files"), tr("List of recently opened memory card files."));
	registerHelp(ui->clearRecentButton, tr("Clear Recent Files"), tr("Remove all entries from the recent files list."));

	connect(ui->browseButton, &QPushButton::clicked, this, &FilesSettingsWidget::onBrowseMemoryCardPath);
	connect(ui->browseImportExportButton, &QPushButton::clicked, this, &FilesSettingsWidget::onBrowseImportExportPath);
	connect(ui->clearRecentButton, &QPushButton::clicked, this, &FilesSettingsWidget::onClearRecentFiles);
	connect(ui->memoryCardPathEdit, &QLineEdit::textChanged, this, &SettingsWidget::settingChanged);
	connect(ui->importExportPathEdit, &QLineEdit::textChanged, this, &SettingsWidget::settingChanged);

	ui->recentFilesList->setSelectionMode(QAbstractItemView::NoSelection);

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

	// Load recent files from config
	ui->recentFilesList->clear();
	const auto& recentFiles = config->getRecentFiles();
	for (const auto& file : recentFiles)
	{
		ui->recentFilesList->addItem(QString::fromStdString(file));
	}

	if (ui->recentFilesList->count() == 0)
	{
		ui->recentFilesList->addItem(tr("No recent files"));
		ui->recentFilesList->setEnabled(false);
		ui->clearRecentButton->setEnabled(false);
	}
	else
	{
		ui->recentFilesList->setEnabled(true);
		ui->clearRecentButton->setEnabled(true);
	}
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

void FilesSettingsWidget::onClearRecentFiles()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Clear Recent Files"),
		tr("Are you sure you want to clear all recent files?"),
		QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::Yes)
	{
		config->clearRecentFiles();
		ui->recentFilesList->clear();
		ui->recentFilesList->addItem(tr("No recent files"));
		ui->recentFilesList->setEnabled(false);
		ui->clearRecentButton->setEnabled(false);
		emit settingChanged();
	}
}

void FilesSettingsWidget::onBrowseImportExportPath()
{
	QString dir = QFileDialog::getExistingDirectory(this,
		tr("Select Import/Export Folder"), ui->importExportPathEdit->text());
	if (!dir.isEmpty())
	{
		ui->importExportPathEdit->setText(dir);
	}
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
