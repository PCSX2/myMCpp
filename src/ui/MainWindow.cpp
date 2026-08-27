// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MainWindow.h"
#include "MemoryCardBrowser.h"
#include "SaveDetailsPanel.h"
#include "IconWidget.h"
#include "CardActionHandler.h"
#include "NewCardDialog.h"
#include "dialogs/AboutDialog.h"
#include "dialogs/ImportExportSavesDialog.h"
#include "ps2mc.h"
#include "ps2save.h"
#include "Settings/SettingsWindow.h"
#include "DiscordRPCManager.h"
#include "Config.h"
#include "Themes.h"
#include "version.h"
#include "BuildVersion.h"
#include "QtUtils.h"
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QInputDialog>
#include <QFormLayout>

#include "ui_MainWindow.h"

namespace
{
	QString getCardTypeLabelFromPath(const QString& cardPath)
	{
		const QString ext = QFileInfo(cardPath).suffix().toLower();
		for (const PS2MemoryCard::CardFormat& format : PS2MemoryCard::getFormats())
		{
			if (ext == QLatin1String(format.extension))
				return QCoreApplication::translate("CardFormats", format.displayName);
		}
		return QCoreApplication::translate("MainWindow", "Memory Card");
	}

	QString makeCardDisplayLabel(const QString& cardPath)
	{
		if (cardPath.isEmpty())
			return QCoreApplication::translate("MainWindow", "Memory Card");

		return QStringLiteral("%1 (%2)").arg(
			QFileInfo(cardPath).fileName(),
			getCardTypeLabelFromPath(cardPath));
	}

	bool samePath(const QString& a, const QString& b)
	{
		if (a.isEmpty() || b.isEmpty())
			return false;

		const QString pathA = QFileInfo(a).canonicalFilePath();
		const QString pathB = QFileInfo(b).canonicalFilePath();
		return !pathA.isEmpty() && pathA == pathB;
	}

	QString memoryCardOpenFilter()
	{
		QStringList patterns;
		QStringList filters;
		for (const PS2MemoryCard::CardFormat& format : PS2MemoryCard::getFormats())
		{
			patterns.append(QStringLiteral("*.") + QLatin1String(format.extension));
			filters.append(QCoreApplication::translate("CardFormats", format.filterName));
		}
		filters.prepend(QCoreApplication::translate("MainWindow", "All Memory Cards (%1)").arg(patterns.join(QLatin1Char(' '))));
		filters.append(QCoreApplication::translate("MainWindow", "All Files (*.*)"));
		return filters.join(QStringLiteral(";;"));
	}

	QString memoryCardSaveFilter(const QString& preferredExt = {})
	{
		QStringList filters;
		const QString preferred = preferredExt.toLower();
		for (const PS2MemoryCard::CardFormat& format : PS2MemoryCard::getFormats())
		{
			const QString label = QCoreApplication::translate("CardFormats", format.filterName);
			if (preferred == QLatin1String(format.extension))
				filters.prepend(label);
			else
				filters.append(label);
		}
		filters.append(QCoreApplication::translate("MainWindow", "All Files (*.*)"));
		return filters.join(QStringLiteral(";;"));
	}
} // namespace

MainWindow::MainWindow(Config* config, QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, memoryCard(nullptr)
	, m_config(config)
	, m_settingsWindow(nullptr)
	, m_discordRpc(std::make_unique<DiscordRPCManager>(this))
{
	if (m_config)
		Themes::UpdateApplicationTheme(m_config);

	ui->setupUi(this);
	QString baseTitle = QStringLiteral("%1 v%2")
	                        .arg(QString::fromUtf8(MYMCpp_APP_NAME))
	                        .arg(QString::fromUtf8(myMCpp_VERSION_STRING));

	const QString gitTag = QString::fromUtf8(BuildVersion::GitTag);
	const QString gitRev = QString::fromUtf8(BuildVersion::GitRev);
	const QString gitHash = QString::fromUtf8(BuildVersion::GitHash);

	if (!gitTag.isEmpty())
		setWindowTitle(QStringLiteral("%1 (%2)").arg(baseTitle, gitTag));
	else if (!gitRev.isEmpty() && gitRev != QStringLiteral("Unknown"))
		setWindowTitle(QStringLiteral("%1 (%2)").arg(baseTitle, gitRev));
	else if (!gitHash.isEmpty())
		setWindowTitle(QStringLiteral("%1 (git %2)").arg(baseTitle, gitHash.left(7)));
	else
		setWindowTitle(baseTitle);
	ui->detailsPanel->setConfig(config);
	connect(ui->detailsPanel, &SaveDetailsPanel::iconWidgetChanged, this,
		[this](IconWidget* iconWidget) {
			connect(iconWidget, &IconWidget::iconLoadFailed, this,
				[this](const QString& reason) {
					ui->statusBar->showMessage(tr("Icon preview unavailable: %1").arg(reason), 8000);
				});
		});
	actionHandler = std::make_unique<CardActionHandler>(this);

	setWindowIcon(QIcon(":/icons/AppIcon.png"));

	connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpenMemoryCard);
	connect(ui->actionCreate, &QAction::triggered, this, &MainWindow::onCreateMemoryCard);
	connect(ui->actionClose, &QAction::triggered, this, &MainWindow::onCloseMemoryCard);
	connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::onSaveAs);
	connect(ui->actionCardInfo, &QAction::triggered, this, &MainWindow::onCardInfo);
	connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
	connect(ui->actionImport, &QAction::triggered, this, &MainWindow::onImportSave);
	connect(ui->actionExport, &QAction::triggered, this, &MainWindow::onExportSave);
	connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::onDeleteFile);
	connect(ui->actionSelectAll, &QAction::triggered, this, &MainWindow::onSelectAll);
	connect(ui->actionFormat, &QAction::triggered, this, &MainWindow::onFormatCard);
	connect(ui->actionEccTool, &QAction::triggered, this, &MainWindow::onEccTool);

	ui->actionAscii->setChecked(m_config ? m_config->getAsciiMode() : false);
	connect(ui->actionAscii, &QAction::triggered, this, &MainWindow::onToggleAscii);

	ui->actionForceImport->setChecked(m_config ? m_config->getForceImport() : false);
	connect(ui->actionForceImport, &QAction::triggered, this, &MainWindow::onToggleForceImport);

	if (m_config)
	{
		const bool toolbarLocked = m_config->getToolbarLocked();
		ui->actionLockToolbar->setChecked(toolbarLocked);
		ui->mainToolBar->setMovable(!toolbarLocked);
		ui->mainToolBar->setFloatable(!toolbarLocked);
	}
	else
	{
		ui->mainToolBar->setMovable(true);
		ui->mainToolBar->setFloatable(true);
	}

	connect(ui->actionLockToolbar, &QAction::triggered, this, &MainWindow::onToggleToolbarLock);

	ui->actionAbout->setMenuRole(QAction::AboutRole);
	ui->actionPreferences->setText(tr("Settings..."));
	ui->actionPreferences->setMenuRole(QAction::PreferencesRole);
	ui->actionAboutQt->setMenuRole(QAction::AboutQtRole);

	connect(ui->actionPreferences, &QAction::triggered, this, &MainWindow::onPreferences);
	connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onAbout);
	connect(ui->actionGitHub, &QAction::triggered, this, &MainWindow::onGitHubRepository);
	connect(ui->actionDocumentation, &QAction::triggered, this, &MainWindow::onDocumentation);
	connect(ui->actionCheckUpdates, &QAction::triggered, this, &MainWindow::onCheckForUpdates);
	connect(ui->actionAboutQt, &QAction::triggered, this, &MainWindow::onAboutQt);

	connect(ui->cardBrowser, &QTreeWidget::itemSelectionChanged,
		this, &MainWindow::onCardItemSelected);
	connect(ui->cardBrowser, &QTreeWidget::currentItemChanged,
		this, &MainWindow::onCardItemSelected);
	connect(ui->cardBrowser, &QTreeWidget::itemDoubleClicked,
		this, &MainWindow::onCardItemDoubleClicked);

	connect(ui->cardBrowser, &MemoryCardBrowser::editTimestampRequested,
		this, &MainWindow::onEditTimestamp);

	connect(ui->cardBrowser, &MemoryCardBrowser::renameRequested,
		this, &MainWindow::onRenameEntry);

	connect(ui->cardBrowser, &MemoryCardBrowser::saveFilesDropped, this, &MainWindow::importSaveFiles);

	connect(ui->cardBrowser, &MemoryCardBrowser::exportSaveRequested, this, [this](const QString& savePath) {
		if (!memoryCard)
			return;

		ImportExportSavesDialog nameDialog({savePath}, this);
		if (nameDialog.exec() != QDialog::Accepted)
			return;

		QStringList filenames = nameDialog.getFilenames();
		if (filenames.isEmpty())
			return;
		QString suggestedName = filenames[0];
		QString filter = suggestedName.endsWith(QLatin1String(".max"), Qt::CaseInsensitive) ? tr("MAX Drive Format (*.max);;EMS/PSU Format (*.psu);;All Files (*.*)") : tr("EMS/PSU Format (*.psu);;MAX Drive Format (*.max);;All Files (*.*)");
		const QString exportDir = QtUtils::resolveConfigFolderPath(m_config->getImportExportFolder());
		QString path = QFileDialog::getSaveFileName(this, tr("Export Save"),
			exportDir + QLatin1Char('/') + suggestedName, filter);
		if (!path.isEmpty())
			actionHandler->exportSave(memoryCard.get(), savePath, path);
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::exportFileRequested, this, [this](const QString& savePath, const QString& fileName) {
		if (!memoryCard)
			return;

		const QString exportDir = QtUtils::resolveConfigFolderPath(m_config->getImportExportFolder());
		QString filename = QFileDialog::getSaveFileName(
			this,
			tr("Export File"),
			exportDir + QLatin1Char('/') + fileName,
			tr("All Files (*.*)"));

		if (!filename.isEmpty())
		{
			try
			{
				QString fullPath = savePath + "/" + fileName;
				memoryCard->exportFile(fullPath.toStdString(), filename.toStdString());
				ui->statusBar->showMessage(tr("Exported %1").arg(fileName), 5000);
			}
			catch (const std::exception& e)
			{
				QMessageBox::critical(this, tr("Error"),
					tr("Failed to export file: %1").arg(e.what()));
			}
		}
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::exportSaveAsFolderRequested, this, [this](const QString& savePath) {
		if (!memoryCard)
			return;

		const QString exportDir = QtUtils::resolveConfigFolderPath(m_config->getImportExportFolder());
		QString targetDir = QFileDialog::getExistingDirectory(
			this,
			tr("Export Save as Folder"),
			exportDir);

		if (!targetDir.isEmpty())
			actionHandler->exportSaveAsFolder(memoryCard.get(), savePath, targetDir);
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::exportSavesAsFoldersRequested, this, [this](const QStringList& savePaths) {
		if (!memoryCard || savePaths.isEmpty())
			return;

		const QString exportDir = QtUtils::resolveConfigFolderPath(m_config->getImportExportFolder());
		QString targetDir = QFileDialog::getExistingDirectory(
			this,
			tr("Export Selected as Folders"),
			exportDir);

		if (targetDir.isEmpty())
			return;

		QDir dir(targetDir);
		QStringList existing;
		for (const QString& savePath : savePaths)
		{
			QString saveFolderName = QFileInfo(savePath).fileName();
			saveFolderName = QtUtils::sanitizeFilename(saveFolderName);
			if (dir.exists(saveFolderName))
			{
				existing.append(saveFolderName);
			}
		}

		if (!existing.isEmpty())
		{
			QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Overwrite Folders"),
				tr("%1 folder(s) already exist. Overwrite?").arg(existing.size()),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
			if (reply == QMessageBox::No)
				return;
		}

		int ok = 0, failed = 0;
		QStringList sanitizedChanges;
		QStringList failedNames;
		for (const QString& savePath : savePaths)
		{
			QString rawFolderName = QFileInfo(savePath).fileName();
			QString saveFolderName = QtUtils::sanitizeFilename(rawFolderName);
			if (saveFolderName != rawFolderName)
			{
				sanitizedChanges.append(tr("'%1' -> '%2'").arg(rawFolderName, saveFolderName));
			}
			if (actionHandler->exportSaveAsFolder(memoryCard.get(), savePath, targetDir, false))
				++ok;
			else
			{
				++failed;
				failedNames.append(rawFolderName);
			}
		}

		if (failed > 0)
		{
			QMessageBox::warning(this, tr("Export"),
				tr("Exported %1 folder(s); %2 failed:\n%3").arg(ok).arg(failed).arg(failedNames.join("\n")));
		}
		else
		{
			QMessageBox msgBox(this);
			msgBox.setWindowTitle(tr("Success"));
			QString msgText = tr("Successfully exported %1 folder(s) to:\n%2").arg(ok).arg(targetDir);
			if (!sanitizedChanges.isEmpty())
			{
				msgText += "\n\n" + tr("Note: The following folder names were sanitized due to invalid characters:\n%1").arg(sanitizedChanges.join("\n"));
			}
			msgBox.setText(msgText);
			msgBox.setIcon(QMessageBox::Information);
			QPushButton* openFolderButton = msgBox.addButton(tr("Open Folder"), QMessageBox::ActionRole);
			msgBox.addButton(QMessageBox::Ok);
			msgBox.exec();

			if (msgBox.clickedButton() == openFolderButton)
			{
				QDesktopServices::openUrl(QUrl::fromLocalFile(targetDir));
			}
		}
		ui->statusBar->showMessage(failed == 0 ?
									   tr("Exported %1 folders").arg(ok) :
									   tr("Exported %1, %2 failed").arg(ok).arg(failed),
			5000);
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::exportFilesRequested, this, [this](const QString& savePath, const QStringList& fileNames) {
		if (!memoryCard || fileNames.isEmpty())
			return;

		const QString exportDir = QtUtils::resolveConfigFolderPath(m_config->getImportExportFolder());
		QString targetDir = QFileDialog::getExistingDirectory(
			this,
			tr("Export Selected Files"),
			exportDir);

		if (targetDir.isEmpty())
			return;

		QDir dir(targetDir);
		QStringList existing;
		for (const QString& fileName : fileNames)
		{
			QString cleanFileName = QtUtils::sanitizeFilename(fileName);
			if (dir.exists(cleanFileName))
			{
				existing.append(cleanFileName);
			}
		}

		if (!existing.isEmpty())
		{
			QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Overwrite Files"),
				tr("%1 file(s) already exist in the folder. Overwrite?").arg(existing.size()),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
			if (reply == QMessageBox::No)
				return;
		}

		int ok = 0, failed = 0;
		QStringList sanitizedChanges;
		QStringList failedNames;
		for (const QString& fileName : fileNames)
		{
			try
			{
				QString fullPath = savePath + "/" + fileName;
				QString cleanFileName = QtUtils::sanitizeFilename(fileName);
				if (cleanFileName != fileName)
				{
					sanitizedChanges.append(tr("'%1' -> '%2'").arg(fileName, cleanFileName));
				}
				QString targetFilePath = dir.filePath(cleanFileName);
				memoryCard->exportFile(fullPath.toStdString(), targetFilePath.toStdString());
				++ok;
			}
			catch (const std::exception&)
			{
				++failed;
				failedNames.append(fileName);
			}
		}

		if (failed > 0)
		{
			QMessageBox::warning(this, tr("Export"),
				tr("Exported %1 file(s); %2 failed:\n%3").arg(ok).arg(failed).arg(failedNames.join("\n")));
		}
		else
		{
			QMessageBox msgBox(this);
			msgBox.setWindowTitle(tr("Success"));
			QString msgText = tr("Successfully exported %1 file(s) to:\n%2").arg(ok).arg(targetDir);
			if (!sanitizedChanges.isEmpty())
			{
				msgText += "\n\n" + tr("Note: The following filenames were sanitized due to invalid characters:\n%1").arg(sanitizedChanges.join("\n"));
			}
			msgBox.setText(msgText);
			msgBox.setIcon(QMessageBox::Information);
			QPushButton* openFolderButton = msgBox.addButton(tr("Open Folder"), QMessageBox::ActionRole);
			msgBox.addButton(QMessageBox::Ok);
			msgBox.exec();

			if (msgBox.clickedButton() == openFolderButton)
			{
				QDesktopServices::openUrl(QUrl::fromLocalFile(targetDir));
			}
		}
		ui->statusBar->showMessage(failed == 0 ?
									   tr("Exported %1 files").arg(ok) :
									   tr("Exported %1, %2 failed").arg(ok).arg(failed),
			5000);
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::deleteFilesRequested, this, [this](const QString& savePath, const QStringList& fileNames) {
		deleteSelectedFiles(savePath, fileNames);
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::deleteSaveRequested, this, [this](const QString& savePath) {
		deleteSelectedSave(savePath);
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::importFileRequested, this, [this](const QString& savePath, const QString& hostFilePath) {
		importFileToCard(savePath, hostFilePath);
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::importArbitraryFileRequested, this, [this](const QString& targetDir) {
		if (!memoryCard)
			return;
		const QString importDir = QtUtils::resolveConfigFolderPath(m_config->getImportExportFolder());
		QString fileName = QFileDialog::getOpenFileName(this, tr("Import File"), importDir, tr("All Files (*.*)"));
		if (!fileName.isEmpty())
		{
			importFileToCard(targetDir, fileName);
		}
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::createFolderRequested, this, [this](const QString& parentPath) {
		if (!memoryCard)
		{
			return;
		}

		bool ok;
		QString folderName = QInputDialog::getText(this, tr("New Folder"),
			tr("Folder name:"), QLineEdit::Normal, "", &ok);



		if (ok && !folderName.isEmpty())
		{
			try
			{
				QString fullPath = parentPath == "/" ? "/" + folderName : parentPath + "/" + folderName;
				memoryCard->makeDir(fullPath.toStdString());
				ui->statusBar->showMessage(tr("Created folder %1").arg(folderName), 5000);
				ui->cardBrowser->navigateTo(parentPath);
				updateCardView();
			}
			catch (const std::exception& e)
			{
				QMessageBox::critical(this, tr("Error"),
					tr("Failed to create folder: %1").arg(e.what()));
			}
		}
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::exportMultipleSavesRequested, this, [this](const QStringList& savePaths) {
		if (!memoryCard || savePaths.isEmpty())
			return;

		ImportExportSavesDialog nameDialog(savePaths, this);
		if (nameDialog.exec() != QDialog::Accepted)
			return;

		QStringList filenames = nameDialog.getFilenames();
		const QString exportDir = QtUtils::resolveConfigFolderPath(m_config->getImportExportFolder());
		QString targetDir = QFileDialog::getExistingDirectory(
			this,
			tr("Export Selected Saves"),
			exportDir);

		if (targetDir.isEmpty())
			return;

		QDir dir(targetDir);
		QStringList existing;
		for (int i = 0; i < savePaths.size() && i < filenames.size(); ++i)
		{
			if (!filenames[i].isEmpty())
			{
				QString cleanName = QtUtils::sanitizeFilename(filenames[i]);
				if (QFileInfo(dir, cleanName).exists())
				{
					existing.append(cleanName);
				}
			}
		}
		if (!existing.isEmpty())
		{
			QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Overwrite Files"),
				tr("%1 file(s) already exist in the folder. Overwrite?").arg(existing.size()),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
			if (reply == QMessageBox::No)
				return;
		}

		int ok = 0, failed = 0;
		QStringList sanitizedChanges;
		QStringList failedNames;
		for (int i = 0; i < savePaths.size() && i < filenames.size(); ++i)
		{
			if (filenames[i].isEmpty())
				continue;
			QString rawName = filenames[i];
			QString cleanName = QtUtils::sanitizeFilename(rawName);
			if (cleanName != rawName)
			{
				sanitizedChanges.append(tr("'%1' -> '%2'").arg(rawName, cleanName));
			}
			QString outputPath = dir.filePath(cleanName);
			if (actionHandler->exportSave(memoryCard.get(), savePaths[i], outputPath, false))
				++ok;
			else
			{
				++failed;
				failedNames.append(rawName);
			}
		}

		if (failed > 0)
		{
			QMessageBox::warning(this, tr("Export"), tr("Exported %1 save(s); %2 failed:\n%3").arg(ok).arg(failed).arg(failedNames.join("\n")));
		}
		else
		{
			QMessageBox msgBox(this);
			msgBox.setWindowTitle(tr("Success"));
			QString msgText = tr("Successfully exported %1 save(s) to:\n%2").arg(ok).arg(targetDir);
			if (!sanitizedChanges.isEmpty())
			{
				msgText += "\n\n" + tr("Note: The following filenames were sanitized due to invalid characters:\n%1").arg(sanitizedChanges.join("\n"));
			}
			msgBox.setText(msgText);
			msgBox.setIcon(QMessageBox::Information);
			QPushButton* openFolderButton = msgBox.addButton(tr("Open Folder"), QMessageBox::ActionRole);
			msgBox.addButton(QMessageBox::Ok);
			msgBox.exec();

			if (msgBox.clickedButton() == openFolderButton)
			{
				QDesktopServices::openUrl(QUrl::fromLocalFile(targetDir));
			}
		}
		ui->statusBar->showMessage(failed == 0 ? tr("Exported %1 saves").arg(ok) : tr("Exported %1, %2 failed").arg(ok).arg(failed), 5000);
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::deleteMultipleSavesRequested, this, [this](const QStringList& savePaths) {
		deleteSelectedSaves(savePaths);
	});

	actionHandler->setStatusBar(ui->statusBar);

	if (m_config)
	{
		m_discordRpc->setEnabled(m_config->getDiscordRPCEnabled());
		if (m_discordRpc && !currentCardPath.isEmpty())
		{
			m_discordRpc->setCardOpenContext(makeCardDisplayLabel(currentCardPath));
		}
	}

	updateStatusBar();
}

MainWindow::~MainWindow()
{
}

void MainWindow::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
		updateStatusBar();
	}
	QMainWindow::changeEvent(event);
}

void MainWindow::onOpenMemoryCard()
{
	const QString memoryCardDir = QtUtils::resolveConfigFolderPath(m_config->getMemoryCardFolder());
	QString filename = QFileDialog::getOpenFileName(
		this,
		tr("Open Memory Card"),
		memoryCardDir,
		memoryCardOpenFilter());

	if (filename.isEmpty())
	{
		return;
	}

	const bool reloading = memoryCard && !currentCardPath.isEmpty() && samePath(currentCardPath, filename);

	closeCard();

	auto card = actionHandler->openCard(filename);
	if (card)
	{
		memoryCard.reset(card);
		currentCardPath = filename;

		updateCardView();

		ui->actionClose->setEnabled(true);
		ui->actionSaveAs->setEnabled(true);
		ui->actionCardInfo->setEnabled(true);
		const bool hasEcc = memoryCard->hasEcc();
		ui->actionEccTool->setText(hasEcc ? tr("Remove ECC and Save &Copy As...") : tr("Add ECC and Save &Copy As..."));
		ui->actionEccTool->setStatusTip(hasEcc ?
											tr("Remove ECC from the memory card and save a copy to another file") :
											tr("Add ECC to the memory card and save a copy to another file"));
		ui->actionEccTool->setEnabled(true);
		ui->actionImport->setEnabled(true);
		ui->actionExport->setEnabled(true);
		ui->actionFormat->setEnabled(true);
		ui->actionSelectAll->setEnabled(true);

		if (m_discordRpc)
		{
			m_discordRpc->setCardOpenContext(makeCardDisplayLabel(currentCardPath));
		}

		if (reloading)
		{
			ui->statusBar->showMessage(
				tr("Reloaded %1").arg(QFileInfo(filename).fileName()),
				5000);
		}
	}
}

void MainWindow::onCreateMemoryCard()
{
	NewCardDialog optionsDialog(this);
	if (optionsDialog.exec() != QDialog::Accepted)
		return;

	const int sizeMB = optionsDialog.getCardSizeMB();
	const QString ext = optionsDialog.getCardExtension();
	const bool disableEcc = !optionsDialog.usesEcc();
	const QString fileName = optionsDialog.getFileName();
	const QString memoryCardDir = QtUtils::resolveConfigFolderPath(m_config->getMemoryCardFolder());
	QDir().mkpath(memoryCardDir);

	QString filename = QFileDialog::getSaveFileName(
		this,
		tr("Create Memory Card"),
		QDir(memoryCardDir).filePath(fileName),
		memoryCardSaveFilter(ext));

	if (filename.isEmpty())
		return;

	if (QFileInfo(filename).suffix().isEmpty())
		filename += QLatin1Char('.') + ext;

	if (QFile::exists(filename))
	{
		const auto reply = QMessageBox::warning(
			this,
			tr("Create Memory Card"),
			tr("'%1' already exists.\n\n"
			   "Creating a memory card here will overwrite that file and erase any saves on it.\n\n"
			   "Continue?")
				.arg(QDir::toNativeSeparators(filename)),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::No);

		if (reply != QMessageBox::Yes)
			return;
	}

	closeCard();

	auto card = actionHandler->createCard(filename, sizeMB, disableEcc);
	if (card)
	{
		memoryCard.reset(card);
		currentCardPath = filename;

		updateCardView();

		ui->actionClose->setEnabled(true);
		ui->actionSaveAs->setEnabled(true);
		ui->actionCardInfo->setEnabled(true);
		const bool hasEcc = memoryCard->hasEcc();
		ui->actionEccTool->setText(hasEcc ? tr("Remove ECC and Save &Copy As...") : tr("Add ECC and Save &Copy As..."));
		ui->actionEccTool->setStatusTip(hasEcc ?
											tr("Remove ECC from the memory card and save a copy to another file") :
											tr("Add ECC to the memory card and save a copy to another file"));
		ui->actionEccTool->setEnabled(true);
		ui->actionImport->setEnabled(true);
		ui->actionExport->setEnabled(true);
		ui->actionFormat->setEnabled(true);
		ui->actionSelectAll->setEnabled(true);

		if (m_discordRpc)
		{
			m_discordRpc->setCardOpenContext(makeCardDisplayLabel(currentCardPath));
		}

		ui->statusBar->showMessage(tr("Created %1").arg(QFileInfo(filename).fileName()), 5000);
	}
}

void MainWindow::onCloseMemoryCard()
{
	closeCard();
}

void MainWindow::onImportSave()
{
	if (!memoryCard)
	{
		return;
	}

	const QString importDir = QtUtils::resolveConfigFolderPath(m_config->getImportExportFolder());
	QStringList filenames = QFileDialog::getOpenFileNames(
		this,
		tr("Import Save"),
		importDir,
		tr("PS2 Save Files (*.psu *.max *.sps *.xps *.cbs *.psv);;EMS/PSU (*.psu);;MAX Drive (*.max);;SharkPort (*.sps);;X-Port (*.xps);;CodeBreaker (*.cbs);;PSV (*.psv);;All Files (*.*)"));

	if (!filenames.isEmpty())
	{
		importSaveFiles(filenames);
	}
}

void MainWindow::importSaveFiles(const QStringList& paths)
{
	if (!memoryCard || paths.isEmpty())
		return;

	bool forceOverwrite = m_config ? m_config->getForceImport() : false;

	QList<ImportExportSavesDialog::ImportItem> importItems;
	for (const QString& filepath : paths)
	{
		ImportExportSavesDialog::ImportItem item;
		item.filePath = filepath;

		try
		{
			auto save = std::make_shared<PS2SaveFile>();
			save->load(filepath.toStdString());

			std::string saveName = save->getTitle();
			if (saveName.empty())
			{
				const auto& entries = save->getEntries();
				if (!entries.empty())
				{
					const bool hasDir = (entries[0].dirEntry.mode & DF_DIR) != 0;
					saveName = hasDir ? entries[0].dirEntry.name : "";
				}
				if (saveName.empty())
				{
					saveName = QFileInfo(filepath).baseName().toStdString();
				}
			}

			item.gameTitle = QString::fromStdString(saveName);

			const auto& entries = save->getEntries();
			std::string dirId;
			if (!entries.empty())
			{
				const bool hasDirHeader = (entries[0].dirEntry.mode & DF_DIR);
				dirId = hasDirHeader ? entries[0].dirEntry.name : save->getTitle();
				if (dirId.empty())
				{
					dirId = entries[0].dirEntry.name;
				}
			}
			if (dirId.empty())
			{
				dirId = QFileInfo(filepath).baseName().toStdString();
			}

			item.directoryId = QString::fromStdString(dirId);

			if (memoryCard)
			{
				try
				{
					memoryCard->getEntry("/" + dirId);
					item.exists = true;
					item.overwriteSelected = forceOverwrite;
				}
				catch (...)
				{
					item.exists = false;
				}
			}

			item.saveFile = save;
		}
		catch (const std::exception& e)
		{
			item.corrupt = true;
			item.errorText = QString::fromStdString(e.what());
			item.gameTitle = QFileInfo(filepath).fileName();
		}

		importItems.append(item);
	}

	ImportExportSavesDialog dialog(importItems, this);
	if (dialog.exec() != QDialog::Accepted)
	{
		return;
	}

	int ok = 0;
	int skipped = 0;
	int failed = 0;
	QStringList failedNames;

	for (const auto& item : importItems)
	{
		if (!item.importSelected || item.corrupt)
			continue;

		if (item.exists && !item.overwriteSelected)
		{
			skipped++;
			continue;
		}

		CardActionHandler::ImportResult res = actionHandler->importSave(
			memoryCard.get(), item.saveFile, item.filePath, false, item.overwriteSelected);

		if (res == CardActionHandler::ImportResult::Success)
		{
			ok++;
		}
		else
		{
			failed++;
			failedNames.append(QFileInfo(item.filePath).fileName());
		}
	}

	if (failed > 0)
	{
		QMessageBox::warning(this, tr("Import"),
			tr("Imported %1 save(s); %2 failed:\n%3").arg(ok).arg(failed).arg(failedNames.join("\n")));
	}
	else if (ok > 0)
	{
		if (skipped > 0)
		{
			QMessageBox::information(this, tr("Import"),
				tr("Successfully imported %1 save(s) (skipped %2 duplicate(s))").arg(ok).arg(skipped));
		}
		else
		{
			QMessageBox::information(this, tr("Import"),
				tr("Successfully imported %1 save(s)").arg(ok));
		}
	}

	if (failed > 0)
		ui->statusBar->showMessage(tr("Imported %1, %2 failed").arg(ok).arg(failed), 8000);
	else if (ok > 0)
		ui->statusBar->showMessage(tr("Imported %1 save(s)").arg(ok), 8000);
	else
		ui->statusBar->showMessage(tr("No saves imported"), 8000);

	if (m_discordRpc && ok > 0)
	{
		m_discordRpc->setTemporaryPresence(
			tr("Card: %1").arg(makeCardDisplayLabel(currentCardPath)),
			tr("Imported %1 Saves").arg(ok));
	}

	updateCardView();
}

void MainWindow::onExportSave()
{
	if (!memoryCard || !ui->cardBrowser->hasSaveSelected())
	{
		QMessageBox::information(this, tr("Export"),
			tr("Please select a save to export"));
		return;
	}

	const QString exportDir = QtUtils::resolveConfigFolderPath(m_config->getImportExportFolder());
	QString filename = QFileDialog::getSaveFileName(
		this,
		tr("Export Save"),
		exportDir,
		tr("EMS/PSU Format (*.psu);;MAX Drive Format (*.max);;All Files (*.*)"));

	if (filename.isEmpty())
	{
		return;
	}

	QString savePath = ui->cardBrowser->getCurrentSavePath();
	actionHandler->exportSave(memoryCard.get(), savePath, filename);
	if (m_discordRpc)
	{
		m_discordRpc->setTemporaryPresence(
			tr("Card: %1").arg(makeCardDisplayLabel(currentCardPath)),
			tr("Exported Save: %1").arg(QFileInfo(filename).fileName()));
	}
}

void MainWindow::onDeleteFile()
{
	if (!memoryCard)
		return;

	if (ui->cardBrowser->isInsideSave())
	{
		const QStringList selectedPaths = ui->cardBrowser->getSelectedPaths();
		if (selectedPaths.isEmpty())
		{
			QMessageBox::information(this, tr("Delete"),
				tr("Please select a file or folder to delete"));
			return;
		}

		QStringList fileNames;
		for (const QString& path : selectedPaths)
			fileNames.append(QFileInfo(path).fileName());

		deleteSelectedFiles(ui->cardBrowser->getCurrentPath(), fileNames);
		return;
	}

	const QStringList savePaths = ui->cardBrowser->getSelectedSavePaths();
	if (savePaths.isEmpty())
	{
		QMessageBox::information(this, tr("Delete"),
			tr("Please select a file or folder to delete"));
		return;
	}

	if (savePaths.size() == 1)
		deleteSelectedSave(savePaths.first());
	else
		deleteSelectedSaves(savePaths);
}

void MainWindow::deleteSelectedSave(const QString& savePath)
{
	if (!memoryCard || savePath.isEmpty())
		return;

	if (m_config && m_config->getWarnOnDelete())
	{
		QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Delete"),
			tr("Are you sure you want to delete '%1'?\nThis action cannot be undone.").arg(savePath),
			QMessageBox::Yes | QMessageBox::No);

		if (reply == QMessageBox::No)
			return;
	}

	if (!actionHandler->deleteSave(memoryCard.get(), savePath))
		return;

	if (m_discordRpc)
	{
		m_discordRpc->setTemporaryPresence(
			tr("Card: %1").arg(makeCardDisplayLabel(currentCardPath)),
			tr("Deleted: %1").arg(QFileInfo(savePath).fileName()));
	}

	updateCardView();
	ui->detailsPanel->clear();
}

void MainWindow::deleteSelectedSaves(const QStringList& savePaths)
{
	if (!memoryCard || savePaths.isEmpty())
		return;

	if (m_config && m_config->getWarnOnDelete())
	{
		QStringList displayNames;
		for (const QString& path : savePaths)
		{
			QString name = path;
			if (name.startsWith('/'))
				name.remove(0, 1);
			displayNames.append(name);
		}

		QMessageBox::StandardButton reply = QMessageBox::question(
			this,
			tr("Delete Selected Saves"),
			tr("Are you sure you want to delete the following saves?\n\n%1").arg(displayNames.join("\n")),
			QMessageBox::Yes | QMessageBox::No);

		if (reply == QMessageBox::No)
			return;
	}

	int ok = 0, failed = 0;
	QStringList failedNames;
	for (const QString& savePath : savePaths)
	{
		if (actionHandler->deleteSave(memoryCard.get(), savePath, false))
		{
			++ok;
		}
		else
		{
			++failed;
			QString name = savePath;
			if (name.startsWith('/'))
				name.remove(0, 1);
			failedNames.append(name);
		}
	}

	if (failed > 0)
	{
		QMessageBox::warning(this, tr("Delete"),
			tr("Deleted %1 save(s); %2 failed:\n%3").arg(ok).arg(failed).arg(failedNames.join("\n")));
	}

	ui->statusBar->showMessage(failed == 0 ?
								   tr("Deleted %1 saves").arg(ok) :
								   tr("Deleted %1, %2 failed").arg(ok).arg(failed),
		5000);

	if (m_discordRpc && ok > 0)
	{
		m_discordRpc->setTemporaryPresence(
			tr("Card: %1").arg(makeCardDisplayLabel(currentCardPath)),
			tr("Deleted %1 Saves").arg(ok));
	}

	updateCardView();
	ui->detailsPanel->clear();
}

void MainWindow::deleteSelectedFiles(const QString& parentPath, const QStringList& fileNames)
{
	if (!memoryCard || fileNames.isEmpty())
		return;

	if (m_config && m_config->getWarnOnDelete())
	{
		const QString message = fileNames.size() == 1 ?
		                            tr("Are you sure you want to delete '%1'?").arg(fileNames.first()) :
		                            tr("Are you sure you want to delete the %1 selected file(s)?").arg(fileNames.size());
		QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Delete Files"),
			message,
			QMessageBox::Yes | QMessageBox::No);

		if (reply == QMessageBox::No)
			return;
	}

	int ok = 0, failed = 0;
	QStringList failedNames;
	for (const QString& name : fileNames)
	{
		const QString fullPath = parentPath + "/" + name;
		if (actionHandler->deleteSave(memoryCard.get(), fullPath, false))
		{
			++ok;
		}
		else
		{
			++failed;
			failedNames.append(name);
		}
	}

	if (failed > 0)
	{
		QMessageBox::warning(this, tr("Delete"),
			tr("Deleted %1 file(s); %2 failed:\n%3").arg(ok).arg(failed).arg(failedNames.join("\n")));
	}

	ui->statusBar->showMessage(failed == 0 ?
								   tr("Deleted %1 files").arg(ok) :
								   tr("Deleted %1, %2 failed").arg(ok).arg(failed),
		5000);

	updateCardView();
	ui->cardBrowser->navigateTo(parentPath);
	ui->detailsPanel->clear();
}

void MainWindow::onFormatCard()
{
	if (!memoryCard)
	{
		return;
	}

	actionHandler->formatCard(memoryCard.get(), currentCardPath);
	if (m_discordRpc)
	{
		m_discordRpc->setTemporaryPresence(
			tr("Card: %1").arg(makeCardDisplayLabel(currentCardPath)),
			tr("Formatted Memory Card"));
	}
	updateCardView();
}

void MainWindow::onSettingsChanged()
{
	if (ui->detailsPanel)
	{
		ui->detailsPanel->refreshConfig();
	}

	updateForceImportWarning();
	updateCardView();

	if (m_config && m_discordRpc)
	{
		m_discordRpc->setEnabled(m_config->getDiscordRPCEnabled());
		if (!currentCardPath.isEmpty())
		{
			m_discordRpc->setCardOpenContext(makeCardDisplayLabel(currentCardPath));
		}
		else
		{
			m_discordRpc->resetToIdle();
		}
	}
}

void MainWindow::onLanguageChanged()
{
	const bool hadSettingsWindow = (m_settingsWindow != nullptr);

	if (ui)
	{
		ui->retranslateUi(this);
		updateStatusBar();
		updateForceImportWarning();
	}

	if (m_settingsWindow)
	{
		m_settingsWindow->disconnect();
		m_settingsWindow->close();
		m_settingsWindow->deleteLater();
		m_settingsWindow = nullptr;
	}

	if (hadSettingsWindow)
	{
		onPreferences();
	}
}

void MainWindow::onPreferences()
{
	if (!m_settingsWindow)
	{
		m_settingsWindow = new SettingsWindow(m_config, this);

		connect(m_settingsWindow, &SettingsWindow::applicationSettingsChanged, this, &MainWindow::onSettingsChanged);
		connect(m_settingsWindow, &SettingsWindow::languageChanged, this, &MainWindow::onLanguageChanged);

		connect(m_settingsWindow, &QDialog::finished, m_settingsWindow, &QObject::deleteLater);
		connect(m_settingsWindow, &QObject::destroyed, this, [this]() {
			m_settingsWindow = nullptr;
		});
	}

	m_settingsWindow->show();
	m_settingsWindow->raise();
	m_settingsWindow->activateWindow();
}

void MainWindow::onAbout()
{
	AboutDialog dialog(this);
	dialog.exec();
}

void MainWindow::onGitHubRepository()
{
	QDesktopServices::openUrl(QUrl("https://github.com/PCSX2/myMCpp"));
}

void MainWindow::onDocumentation()
{
	QDesktopServices::openUrl(QUrl("https://github.com/PCSX2/myMCpp/wiki"));
}

void MainWindow::onCheckForUpdates()
{
	QMessageBox::information(this, tr("Check for Updates"),
		tr("TBD\n\n"
		   "For updates, visit: https://github.com/PCSX2/myMCpp/releases"));
}

void MainWindow::onAboutQt()
{
	QApplication::aboutQt();
}

void MainWindow::onCardItemSelected()
{
	auto getCurrentSaveDisplayName = [this]() -> QString {
		QString currentSavePath = ui->cardBrowser->getCurrentPath();
		if (currentSavePath.startsWith('/'))
		{
			currentSavePath.remove(0, 1);
		}

		QString fallback = currentSavePath.isEmpty() ? tr("Save") : currentSavePath;
		if (!memoryCard || !ui->cardBrowser->isInsideSave())
		{
			return fallback;
		}

		try
		{
			const std::string savePath = ui->cardBrowser->getCurrentPath().toStdString();
			const std::string title = memoryCard->getSaveTitle(savePath);
			const std::string subtitle = memoryCard->getSaveSubtitle(savePath);

			if (!title.empty())
			{
				QString display = QString::fromStdString(title);
				if (!subtitle.empty())
				{
					display += QStringLiteral(" ") + QString::fromStdString(subtitle);
				}
				return display;
			}
		}
		catch (...)
		{
		}

		return fallback;
	};

	QTreeWidgetItem* item = ui->cardBrowser->currentItem();
	if (!item)
	{
		const auto selected = ui->cardBrowser->selectedItems();
		if (!selected.isEmpty())
		{
			item = selected.first();
		}
	}

	const QStringList selectedPaths = ui->cardBrowser->getSelectedPaths();
	if (!memoryCard || (selectedPaths.isEmpty() && !item))
	{
		if (!ui->cardBrowser->isInsideSave())
		{
			ui->detailsPanel->clear();
		}
		ui->actionDelete->setEnabled(false);
		ui->actionExport->setEnabled(false);
		if (m_discordRpc && !currentCardPath.isEmpty())
		{
			if (ui->cardBrowser->isInsideSave())
			{
				m_discordRpc->setPresenceText(
					tr("Inside: %1").arg(getCurrentSaveDisplayName()),
					tr("Browsing Files"));
			}
			else
			{
				m_discordRpc->setCardOpenContext(makeCardDisplayLabel(currentCardPath));
			}
		}
		return;
	}

	if (!item)
	{
		return;
	}

	QString savePath = ui->cardBrowser->getCurrentSavePath();
	if (savePath.isEmpty() && item)
	{
		QString name = item->data(0, Qt::UserRole).toString();
		if (!name.isEmpty() && name != "..")
		{
			savePath = (ui->cardBrowser->getCurrentPath() == "/") ? ("/" + name) : ui->cardBrowser->getCurrentPath();
		}
	}

	QString size = item->text(1);
	QString modified = item->text(2);

	ui->detailsPanel->setSave(memoryCard.get(), savePath, size, modified);

	if (m_discordRpc)
	{
		const QString highlighted = item->text(0).trimmed().isEmpty() ? item->data(0, Qt::UserRole).toString() : item->text(0);
		const bool isDir = item->data(0, Qt::UserRole + 1).toBool();
		if (ui->cardBrowser->isInsideSave())
		{
			const QString state = isDir ? tr("Selected Folder: %1").arg(highlighted) : tr("Selected File: %1").arg(highlighted);
			m_discordRpc->setPresenceText(
				tr("Inside: %1").arg(getCurrentSaveDisplayName()),
				state);
		}
		else
		{
			const QString state = isDir ? tr("Highlighted Save: %1").arg(highlighted) : tr("Highlighted Item: %1").arg(highlighted);
			m_discordRpc->setPresenceText(
				tr("Card: %1").arg(makeCardDisplayLabel(currentCardPath)),
				state);
		}
	}

	ui->actionDelete->setEnabled(true);
	ui->actionExport->setEnabled(true);
}

void MainWindow::onCardItemDoubleClicked()
{
	QTreeWidgetItem* item = ui->cardBrowser->currentItem();
	if (!item)
		return;

	QString name = item->data(0, Qt::UserRole).toString();
	bool isDir = item->data(0, Qt::UserRole + 1).toBool();

	if (name == "..")
	{
		ui->cardBrowser->navigateUp();
		return;
	}

	if (isDir)
	{
		QString currentPath = ui->cardBrowser->getCurrentPath();
		QString targetPath = currentPath == "/" ? "/" + name : currentPath + "/" + name;
		ui->cardBrowser->navigateTo(targetPath);
	}
}

void MainWindow::onEditTimestamp(const QString& mcPath)
{
	if (!memoryCard)
		return;

	try
	{
		const std::string path = mcPath.toStdString();
		PS2McDirEntry entry = memoryCard->getEntry(path);
		std::time_t currentTime = todToTime(entry.modified);

		QDateTime currentDateTime;
		if (currentTime == 0)
		{
			currentDateTime = QDateTime::currentDateTime();
		}
		else
		{
			currentDateTime = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(currentTime));
		}

		QDialog dialog(this);
		dialog.setWindowTitle(tr("Edit Modified Date"));

		QVBoxLayout* layout = new QVBoxLayout(&dialog);

		QLabel* pathLabel = new QLabel(mcPath, &dialog);
		pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
		layout->addWidget(pathLabel);

		QDateTimeEdit* dateTimeEdit = new QDateTimeEdit(currentDateTime, &dialog);
		dateTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm");
		dateTimeEdit->setCalendarPopup(true);
		layout->addWidget(dateTimeEdit);

		QPushButton* nowButton = new QPushButton(tr("Set to Now"), &dialog);
		connect(nowButton, &QPushButton::clicked, &dialog, [dateTimeEdit]() {
			dateTimeEdit->setDateTime(QDateTime::currentDateTime());
		});
		layout->addWidget(nowButton);

		QDialogButtonBox* buttonBox = new QDialogButtonBox(
			QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
		layout->addWidget(buttonBox);

		connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

		if (dialog.exec() != QDialog::Accepted)
			return;

		QDateTime newDateTime = dateTimeEdit->dateTime();
		if (!newDateTime.isValid())
			return;

		std::time_t newTime = static_cast<std::time_t>(newDateTime.toSecsSinceEpoch());

		memoryCard->setModifiedTime(path, newTime);

		updateCardView();
		ui->cardBrowser->navigateTo(ui->cardBrowser->getCurrentPath());
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(this, tr("Error"),
			tr("Failed to edit timestamp: %1").arg(e.what()));
	}
}

void MainWindow::updateCardView()
{
	ui->cardBrowser->loadCard(memoryCard.get());
	updateStatusBar();
}

void MainWindow::updateStatusBar()
{
	if (memoryCard)
	{
		try
		{
			uint32_t freeSpace = memoryCard->getFreeSpace();
			uint32_t allocatable = memoryCard->getAllocatableSpace();
			double freeMB = freeSpace / (1024.0 * 1024.0);
			double totalMB = allocatable / (1024.0 * 1024.0);
			bool hasEcc = memoryCard->hasEcc();
			QString eccLabel = hasEcc ? tr("ECC") : tr("No ECC");
			ui->statusBar->showMessage(
				tr("Free: %2 MB / Total: %1 MB (%3)")
					.arg(totalMB, 0, 'f', 2)
					.arg(freeMB, 0, 'f', 2)
					.arg(eccLabel));
		}
		catch (...)
		{
			ui->statusBar->showMessage(tr("Memory card open"));
		}
	}
	else
	{
		ui->statusBar->showMessage(tr("No memory card open"));
	}
}

void MainWindow::closeCard()
{
	memoryCard.reset();
	currentCardPath.clear();

	ui->cardBrowser->clear();
	ui->detailsPanel->clear();

	ui->actionClose->setEnabled(false);
	ui->actionSaveAs->setEnabled(false);
	ui->actionCardInfo->setEnabled(false);
	ui->actionEccTool->setEnabled(false);
	ui->actionImport->setEnabled(false);
	ui->actionExport->setEnabled(false);
	ui->actionFormat->setEnabled(false);
	ui->actionDelete->setEnabled(false);
	ui->actionSelectAll->setEnabled(false);

	actionHandler->closeCard();
	updateStatusBar();

	if (m_discordRpc)
	{
		m_discordRpc->resetToIdle();
	}
}

void MainWindow::onSaveAs()
{
	if (!memoryCard)
		return;

	const QString memoryCardDir = QtUtils::resolveConfigFolderPath(m_config->getMemoryCardFolder());
	const QFileInfo cardInfo(currentCardPath);
	const QString suffix = cardInfo.suffix().isEmpty() ? "ps2" : cardInfo.suffix();
	const QString defaultPath = memoryCardDir + QLatin1Char('/') + cardInfo.completeBaseName() + "_copy." + suffix;

	QString filename = QFileDialog::getSaveFileName(
		this,
		tr("Save Copy As..."),
		defaultPath,
		memoryCardSaveFilter());

	if (filename.isEmpty())
		return;

	if (samePath(currentCardPath, filename))
	{
		QMessageBox::information(
			this,
			tr("Save Copy As"),
			tr("That file is already open.\n\n"
			   "Pick another name if you want a copy."));
		return;
	}

	try
	{
		memoryCard->saveAs(filename.toStdString(), memoryCard->hasEcc());
		if (m_discordRpc)
		{
			m_discordRpc->setTemporaryPresence(
				tr("Card: %1").arg(makeCardDisplayLabel(currentCardPath)),
				tr("Copy saved: %1").arg(QFileInfo(filename).fileName()));
		}
		ui->statusBar->showMessage(tr("Copy saved to %1").arg(filename), 5000);
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(this, tr("Error"),
			tr("Failed to save: %1").arg(e.what()));
	}
}

void MainWindow::onCardInfo()
{
	if (!memoryCard)
		return;

	try
	{
		PS2MemoryCard::CardInfo info = memoryCard->getCardInfo();

		QDialog dialog(this);
		dialog.setWindowTitle(tr("Card Info"));

		QFormLayout* layout = new QFormLayout(&dialog);

		auto addRow = [layout](const QString& label, const QString& value) {
			layout->addRow(new QLabel(label), new QLabel(value));
		};

		auto addSection = [layout](const QString& label) {
			QLabel* header = new QLabel(label);
			header->setStyleSheet(QStringLiteral("font-weight: bold; margin-top: 8px;"));
			layout->addRow(header, new QLabel);
		};

		const double imageMiB = static_cast<double>(info.imageSizeBytes) / (1024.0 * 1024.0);
		const double usedMiB = static_cast<double>(info.usedBytes) / (1024.0 * 1024.0);
		const double freeMiB = static_cast<double>(info.freeBytes) / (1024.0 * 1024.0);

		addSection(tr("Header"));
		addRow(tr("Magic"), QString::fromLatin1(PS2MC_MAGIC));
		addRow(tr("Image Size"), tr("%1 MiB").arg(imageMiB, 0, 'f', 2));

		addSection(tr("Geometry"));
		addRow(tr("Page Length"), tr("%1 bytes").arg(info.pageSize));
		addRow(tr("Page Physical"), tr("%1 bytes").arg(info.rawPageSize));
		addRow(tr("Pages / Cluster"), QString::number(info.pagesPerCluster));
		addRow(tr("Cluster Size"), tr("%1 KiB").arg(info.clusterSize / 1024.0, 0, 'f', 2));
		addRow(tr("Clusters / Card"), QString::number(info.clustersPerCard));

		addSection(tr("Type and Flags"));
		QString typeLabel = tr("Unknown (%1)").arg(info.cardType);
		if (info.cardType == 2)
			typeLabel = tr("PS2 (2)");
		else if (info.cardType == 1)
			typeLabel = tr("PSX (1)");
		addRow(tr("Card Type"), typeLabel);

		QStringList flagBits;
		if (info.cardFlags & 0x01)
			flagBits << tr("USE_ECC");
		if (info.cardFlags & 0x08)
			flagBits << tr("BAD_BLOCK");
		if (info.cardFlags & 0x10)
			flagBits << tr("ERASE_ZEROES");
		QString flagsText = tr("0x%1").arg(QString::number(info.cardFlags, 16).toUpper());
		if (!flagBits.isEmpty())
			flagsText += tr(" (%1)").arg(flagBits.join(tr(", ")));
		addRow(tr("Card Flags"), flagsText);

		addRow(tr("ECC Layout"), info.withEcc ? tr("Yes (512+16)") : tr("No (512 only)"));

		addSection(tr("Allocation"));
		addRow(tr("Alloc Offset"), QString::number(info.allocatableOffset));
		addRow(tr("Alloc Count"), QString::number(info.allocatableCount));
		addRow(tr("Reserved Clusters"), QString::number(info.reservedClusters));
		addRow(tr("Root Dir Cluster"), QString::number(info.rootDirCluster));
		addRow(tr("Backup Block 1"), QString::number(info.backupBlock1));
		addRow(tr("Backup Block 2"), QString::number(info.backupBlock2));
		addRow(tr("Bad Blocks"), QString::number(info.badBlockCount));

		addSection(tr("Usage"));
		addRow(tr("Used / Free Clusters"),
			tr("%1 / %2").arg(info.usedClusters).arg(info.freeClusters));
		addRow(tr("Used Bytes"), tr("%1 MiB").arg(usedMiB, 0, 'f', 2));
		addRow(tr("Free Bytes"), tr("%1 MiB").arg(freeMiB, 0, 'f', 2));

		addSection(tr("Health"));
		const bool fsOk = memoryCard->check();
		addRow(tr("Filesystem Check"), fsOk ? tr("OK") : tr("Problems detected"));

		dialog.exec();
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(this, tr("Error"),
			tr("Failed to read card info: %1").arg(e.what()));
	}
}

void MainWindow::onSelectAll()
{
	if (!memoryCard)
		return;

	ui->cardBrowser->selectAll();
}

void MainWindow::onEccTool()
{
	if (!memoryCard)
		return;

	bool hasEcc = memoryCard->hasEcc();

	const QString memoryCardDir = QtUtils::resolveConfigFolderPath(m_config->getMemoryCardFolder());
	const QFileInfo cardInfo(currentCardPath);
	const QString prefix = hasEcc ? "NoECC_" : "ECC_";
	const QString defaultPath = memoryCardDir + QLatin1Char('/') + prefix + cardInfo.fileName();

	QString dialogTitle = hasEcc ? tr("Remove ECC and Save Copy As...") : tr("Add ECC and Save Copy As...");

	QString filename = QFileDialog::getSaveFileName(
		this,
		dialogTitle,
		defaultPath,
		memoryCardOpenFilter());

	if (filename.isEmpty())
		return;

	if (samePath(currentCardPath, filename))
	{
		QMessageBox::information(
			this,
			dialogTitle,
			tr("That file is already open.\n\n"
			   "Pick another name for the ECC copy."));
		return;
	}

	try
	{
		memoryCard->saveAs(filename.toStdString(), !hasEcc);
		if (m_discordRpc)
		{
			m_discordRpc->setTemporaryPresence(
				tr("Card: %1").arg(makeCardDisplayLabel(currentCardPath)),
				hasEcc ? tr("Removed ECC") : tr("Added ECC"));
		}
		ui->statusBar->showMessage(tr("ECC copy saved to %1").arg(filename), 5000);
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(this, tr("Error"),
			tr("Failed to save: %1").arg(e.what()));
	}
}

void MainWindow::onRenameEntry(const QString& mcPath)
{
	if (!memoryCard)
		return;

	QString currentName = mcPath;
	int lastSlash = currentName.lastIndexOf('/');
	if (lastSlash >= 0)
		currentName = currentName.mid(lastSlash + 1);

	bool ok = false;
	QString newName = QInputDialog::getText(
		this,
		tr("Rename"),
		tr("New name:"),
		QLineEdit::Normal,
		currentName,
		&ok);

	if (!ok || newName.isEmpty())
		return;

	if (newName == "." || newName == ".." || newName.contains('/'))
	{
		QMessageBox::warning(this, tr("Rename"),
			tr("The name is invalid."));
		return;
	}

	try
	{
		memoryCard->renameEntry(mcPath.toStdString(), newName.toStdString());

		QString parentPath;
		int slash = mcPath.lastIndexOf('/');
		if (slash <= 0)
			parentPath = "/";
		else
			parentPath = mcPath.left(slash);

		updateCardView();
		ui->cardBrowser->navigateTo(parentPath);
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(this, tr("Error"),
			tr("Failed to rename: %1").arg(e.what()));
	}
}

void MainWindow::onToggleAscii()
{
	if (m_config)
	{
		bool newValue = !m_config->getAsciiMode();
		m_config->setAsciiMode(newValue);
		ui->actionAscii->setChecked(newValue);
		m_config->save();
		updateCardView();
	}
}

void MainWindow::onToggleForceImport()
{
	if (m_config)
	{
		bool newValue = !m_config->getForceImport();
		m_config->setForceImport(newValue);
		ui->actionForceImport->setChecked(newValue);
		m_config->save();
		updateForceImportWarning();
	}
}

void MainWindow::onToggleToolbarLock()
{
	if (!m_config)
		return;

	const bool newValue = !m_config->getToolbarLocked();
	m_config->setToolbarLocked(newValue);
	ui->actionLockToolbar->setChecked(newValue);

	if (ui->mainToolBar)
	{
		ui->mainToolBar->setMovable(!newValue);
		ui->mainToolBar->setFloatable(!newValue);
	}

	m_config->save();
}

void MainWindow::importFileToCard(const QString& savePath, const QString& hostFilePath)
{
	if (!memoryCard)
		return;

	try
	{
		QFile file(hostFilePath);
		if (!file.open(QIODevice::ReadOnly))
		{
			QMessageBox::critical(this, tr("Error"),
				tr("Failed to open file: %1").arg(hostFilePath));
			return;
		}

		QByteArray rawBytes = file.readAll();
		std::vector<uint8_t> fileData(rawBytes.begin(), rawBytes.end());

		QFileInfo fileInfo(hostFilePath);
		QString targetPath = savePath;
		if (!targetPath.endsWith('/'))
			targetPath += "/";
		targetPath += fileInfo.fileName();

		memoryCard->writeFile(targetPath.toStdString(), fileData);
		if (m_discordRpc)
		{
			m_discordRpc->setTemporaryPresence(
				tr("Inside: %1").arg(savePath),
				tr("Imported File: %1").arg(fileInfo.fileName()));
		}
		ui->statusBar->showMessage(tr("Imported %1").arg(fileInfo.fileName()), 5000);

		ui->cardBrowser->navigateTo(savePath);
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(this, tr("Error"),
			tr("Failed to import file: %1").arg(e.what()));
	}
}

void MainWindow::updateForceImportWarning()
{
	if (m_config && m_config->getForceImport())
	{
		QPalette palette = ui->statusBar->palette();
		palette.setColor(QPalette::WindowText, Qt::red);
		ui->statusBar->setPalette(palette);
	}
	else
	{
		ui->statusBar->setPalette(QApplication::palette());
	}
	updateStatusBar();
}
