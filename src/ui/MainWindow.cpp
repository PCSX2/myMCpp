// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MainWindow.h"
#include "MemoryCardBrowser.h"
#include "SaveDetailsPanel.h"
#include "CardActionHandler.h"
#include "NewCardDialog.h"
#include "dialogs/AboutDialog.h"
#include "ps2mc.h"
#include "Settings/SettingsWindow.h"
#include "DiscordRPCManager.h"
#include "Config.h"
#include "Themes.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QInputDialog>

#include "ui_MainWindow.h"

namespace
{
	QString getCardTypeLabelFromPath(const QString& cardPath)
	{
		const QString ext = QFileInfo(cardPath).suffix().toLower();
		if (ext == QStringLiteral("ps2"))
			return QStringLiteral("PCSX2");
		if (ext == QStringLiteral("mc2") || ext == QStringLiteral("mcd"))
			return QStringLiteral("MemCard PRO2");
		if (ext == QStringLiteral("vm2") || ext == QStringLiteral("vmc"))
			return QStringLiteral("PS3 VMC");
		if (ext == QStringLiteral("bin") || ext == QStringLiteral("mc"))
			return QStringLiteral("Raw");
		return QStringLiteral("Memory Card");
	}

	QString makeCardDisplayLabel(const QString& cardPath)
	{
		if (cardPath.isEmpty())
			return QStringLiteral("Memory Card");

		return QStringLiteral("%1 (%2)").arg(
			QFileInfo(cardPath).fileName(),
			getCardTypeLabelFromPath(cardPath));
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
	ui->setupUi(this);
	ui->detailsPanel->setConfig(config);
	actionHandler = std::make_unique<CardActionHandler>(this);

	setWindowIcon(QIcon(":/icons/AppIcon.png"));

	connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpenMemoryCard);
	connect(ui->actionCreate, &QAction::triggered, this, &MainWindow::onCreateMemoryCard);
	connect(ui->actionClose, &QAction::triggered, this, &MainWindow::onCloseMemoryCard);
	connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::onSaveAs);
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

	connect(ui->cardBrowser, &QTreeWidget::currentItemChanged,
		this, &MainWindow::onCardItemSelected);
	connect(ui->cardBrowser, &QTreeWidget::itemDoubleClicked,
		this, &MainWindow::onCardItemDoubleClicked);

	connect(ui->cardBrowser, &MemoryCardBrowser::saveFileDropped, this, [this](const QString& path) {
		if (memoryCard)
		{
			actionHandler->importSave(memoryCard.get(), path);
			updateCardView();
			ui->statusBar->showMessage(tr("Imported %1").arg(path), 5000);
		}
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::exportSaveRequested, this, [this](const QString& savePath) {
		if (!memoryCard)
			return;

		QString filename = QFileDialog::getSaveFileName(
			this,
			tr("Export Save"),
			"",
			tr("EMS/PSU Format (*.psu);;MAX Drive Format (*.max);;All Files (*.*)"));

		if (!filename.isEmpty())
		{
			actionHandler->exportSave(memoryCard.get(), savePath, filename);
		}
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::exportFileRequested, this, [this](const QString& savePath, const QString& fileName) {
		if (!memoryCard)
			return;

		QString filename = QFileDialog::getSaveFileName(
			this,
			tr("Export File"),
			fileName,
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

	connect(ui->cardBrowser, &MemoryCardBrowser::deleteSaveRequested, this, [this](const QString& savePath) {
		if (!memoryCard)
			return;

		if (m_config && m_config->getWarnOnDelete())
		{
			QMessageBox::StandardButton reply;
			reply = QMessageBox::question(this, tr("Delete Save"),
				tr("Are you sure you want to delete '%1'?").arg(savePath),
				QMessageBox::Yes | QMessageBox::No);

			if (reply == QMessageBox::No)
				return;
		}

		actionHandler->deleteSave(memoryCard.get(), savePath);
		ui->statusBar->showMessage(tr("Deleted %1").arg(savePath), 5000);
		updateCardView();
		ui->detailsPanel->clear();
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::importFileRequested, this, [this](const QString& savePath, const QString& hostFilePath) {
		importFileToCard(savePath, hostFilePath);
	});

	connect(ui->cardBrowser, &MemoryCardBrowser::importArbitraryFileRequested, this, [this](const QString& targetDir) {
		if (!memoryCard)
			return;
		QString fileName = QFileDialog::getOpenFileName(this, tr("Import File"), "", tr("All Files (*.*)"));
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

	actionHandler->setStatusBar(ui->statusBar);

	if (m_config)
	{
		Themes::UpdateApplicationTheme(m_config);
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
	QString filename = QFileDialog::getOpenFileName(
		this,
		tr("Open Memory Card"),
		"",
		tr("All Memory Cards (*.ps2 *.vm2 *.vmc *.mc2 *.mcd *.bin *.mc);;"
		   "PCSX2 Memory Card (*.ps2);;"
		   "PS3 Virtual Memory Card (*.vm2 *.vmc);;"
		   "MemCard PRO2 (*.mc2 *.mcd);;"
		   "Raw Memory Card (*.bin *.mc);;"
		   "All Files (*.*)"));

	if (filename.isEmpty())
	{
		return;
	}

	closeCard();

	auto card = actionHandler->openCard(filename);
	if (card)
	{
		memoryCard.reset(card);
		currentCardPath = filename;

		updateCardView();

		ui->actionClose->setEnabled(true);
		ui->actionSaveAs->setEnabled(true);
		ui->actionEccTool->setEnabled(true);
		ui->actionImport->setEnabled(true);
		ui->actionExport->setEnabled(true);
		ui->actionFormat->setEnabled(true);
		ui->actionSelectAll->setEnabled(true);

		if (m_discordRpc)
		{
			m_discordRpc->setCardOpenContext(makeCardDisplayLabel(currentCardPath));
		}
	}
}

void MainWindow::onCreateMemoryCard()
{
	NewCardDialog optionsDialog(this);
	if (optionsDialog.exec() != QDialog::Accepted)
	{
		return;
	}

	int sizeMB = optionsDialog.getCardSizeMB();
	bool disableEcc = optionsDialog.getDisableEcc();

	QString filename = QFileDialog::getSaveFileName(
		this,
		tr("Create Memory Card"),
		"Mcd001.ps2",
		tr("PCSX2 Memory Card (*.ps2);;MemCard PRO2 (*.mc2 *.mcd);;All Files (*.*)"));

	if (filename.isEmpty())
	{
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
		ui->actionEccTool->setEnabled(true);
		ui->actionImport->setEnabled(true);
		ui->actionExport->setEnabled(true);
		ui->actionFormat->setEnabled(true);
		ui->actionSelectAll->setEnabled(true);

		if (m_discordRpc)
		{
			m_discordRpc->setCardOpenContext(makeCardDisplayLabel(currentCardPath));
		}

		ui->statusBar->showMessage(tr("Created %1 MB memory card").arg(sizeMB), 5000);
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

	QString filename = QFileDialog::getOpenFileName(
		this,
		tr("Import Save"),
		"",
		tr("PS2 Save Files (*.psu *.max *.sps *.xps *.cbs *.psv);;EMS/PSU (*.psu);;MAX Drive (*.max);;SharkPort (*.sps);;X-Port (*.xps);;CodeBreaker (*.cbs);;PSV (*.psv);;All Files (*.*)"));

	if (filename.isEmpty())
	{
		return;
	}

	actionHandler->importSave(memoryCard.get(), filename);
	if (m_discordRpc)
	{
		m_discordRpc->setTemporaryPresence(
			tr("Card: %1").arg(makeCardDisplayLabel(currentCardPath)),
			tr("Imported Save: %1").arg(QFileInfo(filename).fileName()));
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

	QString filename = QFileDialog::getSaveFileName(
		this,
		tr("Export Save"),
		"",
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
	QString savePath = ui->cardBrowser->getSelectedPath();

	if (savePath.isEmpty())
	{
		QMessageBox::information(this, tr("Delete"),
			tr("Please select a file or folder to delete"));
		return;
	}

	if (m_config && m_config->getWarnOnDelete())
	{
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, tr("Delete"),
			tr("Are you sure you want to delete '%1'?\nThis action cannot be undone.").arg(savePath),
			QMessageBox::Yes | QMessageBox::No);

		if (reply == QMessageBox::No)
			return;
	}

	actionHandler->deleteSave(memoryCard.get(), savePath);
	if (m_discordRpc)
	{
		m_discordRpc->setTemporaryPresence(
			tr("Card: %1").arg(makeCardDisplayLabel(currentCardPath)),
			tr("Deleted: %1").arg(QFileInfo(savePath).fileName()));
	}
	ui->statusBar->showMessage(tr("Deleted %1").arg(savePath), 5000);

	updateCardView();
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

void MainWindow::onPreferences()
{
	if (!m_settingsWindow)
	{
		m_settingsWindow = new SettingsWindow(m_config, this);

		connect(m_settingsWindow, &SettingsWindow::applicationSettingsChanged, this, &MainWindow::onSettingsChanged);

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
	QDesktopServices::openUrl(QUrl("https://github.com/SternXD/myMCpp"));
}

void MainWindow::onDocumentation()
{
	QDesktopServices::openUrl(QUrl("https://github.com/SternXD/myMCpp/wiki"));
}

void MainWindow::onCheckForUpdates()
{
	QMessageBox::information(this, tr("Check for Updates"),
		tr("You are running the latest version of myMCpp (v1.0.0).\n\n"
		   "For updates, visit: https://github.com/SternXD/myMCpp/releases"));
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

	QString selectedPath = ui->cardBrowser->getSelectedPath();
	if (!memoryCard || selectedPath.isEmpty())
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

	QTreeWidgetItem* item = ui->cardBrowser->currentItem();
	if (!item)
	{
		return;
	}

	QString savePath = ui->cardBrowser->getCurrentSavePath();
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
			ui->statusBar->showMessage(tr("Free: %2 MB / Total: %1 MB").arg(totalMB, 0, 'f', 2).arg(freeMB, 0, 'f', 2));
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

	QString filename = QFileDialog::getSaveFileName(
		this,
		tr("Save Memory Card As"),
		QFileInfo(currentCardPath).baseName() + ".ps2",
		tr("PCSX2 Memory Card (*.ps2);;MemCard PRO2 (*.mc2 *.mcd);;All Files (*.*)"));

	if (filename.isEmpty())
		return;

	try
	{
		memoryCard->saveAs(filename.toStdString(), true);
		if (m_discordRpc)
		{
			m_discordRpc->setTemporaryPresence(
				tr("Card: %1").arg(makeCardDisplayLabel(currentCardPath)),
				tr("Saved As: %1").arg(QFileInfo(filename).fileName()));
		}
		ui->statusBar->showMessage(tr("Saved to %1").arg(filename), 5000);
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(this, tr("Error"),
			tr("Failed to save: %1").arg(e.what()));
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

	QString defaultName;
	if (hasEcc)
		defaultName = "NoECC_" + QFileInfo(currentCardPath).fileName();
	else
		defaultName = "ECC_" + QFileInfo(currentCardPath).fileName();

	QString dialogTitle = hasEcc ? tr("Remove ECC and Save As...") : tr("Add ECC and Save As...");

	QString filename = QFileDialog::getSaveFileName(
		this,
		dialogTitle,
		defaultName,
		tr("All Memory Cards (*.ps2 *.vm2 *.vmc *.mc2 *.mcd);;PCSX2 Memory Card (*.ps2);;PS3 Virtual Memory Card (*.vm2 *.vmc);;MemCard PRO2 (*.mc2 *.mcd);;All Files (*.*)"));

	if (filename.isEmpty())
		return;

	try
	{
		memoryCard->saveAs(filename.toStdString(), !hasEcc);
		if (m_discordRpc)
		{
			m_discordRpc->setTemporaryPresence(
				tr("Card: %1").arg(makeCardDisplayLabel(currentCardPath)),
				hasEcc ? tr("Removed ECC") : tr("Added ECC"));
		}
		ui->statusBar->showMessage(tr("Saved to %1").arg(filename), 5000);
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(this, tr("Error"),
			tr("Failed to save: %1").arg(e.what()));
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

		QByteArray data = file.readAll();
		std::vector<uint8_t> fileData(data.begin(), data.end());

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
