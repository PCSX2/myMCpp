// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MainWindow.h"
#include "MemoryCardBrowser.h"
#include "SaveDetailsPanel.h"
#include "CardActionHandler.h"
#include "NewCardDialog.h"
#include "dialogs/AboutDialog.h"
#include "ps2mc.h"
#include "Settings/SettingsWindow.h"
#include "Config.h"
#include "Themes.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QMenuBar>
#include <QDesktopServices>
#include <QUrl>
#include <QStyle>
#include <QLabel>
#include <QFileInfo>

#include "ui_MainWindow.h"

MainWindow::MainWindow(Config* config, QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, memoryCard(nullptr)
	, m_config(config)
	, m_settingsWindow(nullptr)
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
	connect(ui->actionDiscord, &QAction::triggered, this, &MainWindow::onDiscordServer);
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

	actionHandler->setStatusBar(ui->statusBar);

	if (m_config) {
		Themes::UpdateApplicationTheme(m_config);
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
		tr("PS2 Memory Card (*.ps2 *.mc2 *.mcd *.bin *.mc);;All Files (*.*)"));

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
}

void MainWindow::onDeleteFile()
{
	if (!memoryCard || !ui->cardBrowser->hasSaveSelected())
	{
		QMessageBox::information(this, tr("Delete"),
			tr("Please select a save to delete"));
		return;
	}

	QString savePath = ui->cardBrowser->getCurrentSavePath();

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
}

void MainWindow::onFormatCard()
{
	if (!memoryCard)
	{
		return;
	}

	actionHandler->formatCard(memoryCard.get(), currentCardPath);
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

void MainWindow::onDiscordServer()
{
	// Replace TBD with actual invite code when I figure what I'm going to do here
	QDesktopServices::openUrl(QUrl("https://discord.gg"));
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
	if (!memoryCard || !ui->cardBrowser->hasSaveSelected())
	{
		ui->detailsPanel->clear();
		ui->actionDelete->setEnabled(false);
		ui->actionExport->setEnabled(false);
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

	ui->actionDelete->setEnabled(true);
	ui->actionExport->setEnabled(true);
}

void MainWindow::onCardItemDoubleClicked()
{
	if (ui->cardBrowser->hasSaveSelected())
	{
		onExportSave();
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
			ui->statusBar->showMessage(tr("%1 MB / %2 MB Free").arg(totalMB, 0, 'f', 2).arg(freeMB, 0, 'f', 2));
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
		tr("PS2 Memory Card (*.ps2 *.mc2 *.mcd);;All Files (*.*)"));

	if (filename.isEmpty())
		return;

	try
	{
		memoryCard->saveAs(filename.toStdString(), !hasEcc);
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
