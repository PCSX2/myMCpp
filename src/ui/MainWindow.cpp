// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MainWindow.h"
#include "MemoryCardBrowser.h"
#include "SaveDetailsPanel.h"
#include "CardActionHandler.h"
#include "NewCardDialog.h"
#include "ps2mc.h"
#include "SettingsDialog.h"
#include "Config.h"
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

MainWindow::MainWindow(Config* config, QWidget* parent)
	: QMainWindow(parent)
	, memoryCard(nullptr)
	, m_config(config)
{
	actionHandler = std::make_unique<CardActionHandler>(this);

	setupUI();
	createActions();
	createMenus();
	createToolbar();

	setWindowTitle("myMCpp");
	setWindowIcon(QIcon(":/icons/AppIcon.png"));
	resize(900, 600);

	updateStatusBar();
}

MainWindow::~MainWindow()
{
	if (m_config)
		m_config->save();

	closeCard();
}

void MainWindow::setupUI()
{
	QWidget* centralWidget = new QWidget(this);
	setCentralWidget(centralWidget);

	QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

	QSplitter* splitter = new QSplitter(Qt::Horizontal);

	cardBrowser = new MemoryCardBrowser();
	connect(cardBrowser, &QTreeWidget::itemClicked,
		this, &MainWindow::onCardItemSelected);
	connect(cardBrowser, &QTreeWidget::itemDoubleClicked,
		this, &MainWindow::onCardItemDoubleClicked);
	connect(cardBrowser, &MemoryCardBrowser::saveFileDropped, this, [this](const QString& path) {
		if (memoryCard)
		{
			actionHandler->importSave(memoryCard.get(), path);
			updateCardView();
			statusBar->showMessage(tr("Imported %1").arg(path), 5000);
		}
	});
	splitter->addWidget(cardBrowser);

	detailsPanel = new SaveDetailsPanel(m_config);
	splitter->addWidget(detailsPanel);
	splitter->setStretchFactor(0, 2);
	splitter->setStretchFactor(1, 1);

	mainLayout->addWidget(splitter);

	statusBar = new QStatusBar();
	setStatusBar(statusBar);
	actionHandler->setStatusBar(statusBar);
}

void MainWindow::createActions()
{
	openAction = new QAction(tr("&Open Memory Card..."), this);
	openAction->setShortcut(QKeySequence::Open);
	openAction->setStatusTip(tr("Open a PS2 memory card image"));
	connect(openAction, &QAction::triggered, this, &MainWindow::onOpenMemoryCard);

	createAction = new QAction(tr("&Create Memory Card..."), this);
	createAction->setShortcut(QKeySequence::New);
	createAction->setStatusTip(tr("Create a new PS2 memory card image"));
	connect(createAction, &QAction::triggered, this, &MainWindow::onCreateMemoryCard);

	closeAction = new QAction(tr("&Close Memory Card"), this);
	closeAction->setStatusTip(tr("Close the current memory card"));
	closeAction->setEnabled(false);
	connect(closeAction, &QAction::triggered, this, &MainWindow::onCloseMemoryCard);

	saveAsAction = new QAction(tr("&Save As..."), this);
	saveAsAction->setShortcut(QKeySequence::SaveAs);
	saveAsAction->setStatusTip(tr("Save the memory card to a new file"));
	saveAsAction->setEnabled(false);
	connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAs);

	exitAction = new QAction(tr("E&xit"), this);
	exitAction->setShortcut(QKeySequence::Quit);
	exitAction->setStatusTip(tr("Exit the application"));
	connect(exitAction, &QAction::triggered, this, &QWidget::close);

	importAction = new QAction(tr("&Import Save..."), this);
	importAction->setStatusTip(tr("Import a save file to the memory card"));
	importAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
	importAction->setEnabled(false);
	connect(importAction, &QAction::triggered, this, &MainWindow::onImportSave);

	exportAction = new QAction(tr("&Export Save..."), this);
	exportAction->setStatusTip(tr("Export a save file from the memory card"));
	exportAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
	exportAction->setEnabled(false);
	connect(exportAction, &QAction::triggered, this, &MainWindow::onExportSave);

	deleteAction = new QAction(tr("&Delete"), this);
	deleteAction->setShortcut(QKeySequence::Delete);
	deleteAction->setStatusTip(tr("Delete the selected save"));
	deleteAction->setEnabled(false);
	connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteFile);

	selectAllAction = new QAction(tr("Select &All"), this);
	selectAllAction->setShortcut(QKeySequence::SelectAll);
	selectAllAction->setStatusTip(tr("Select all saves"));
	selectAllAction->setEnabled(false);
	connect(selectAllAction, &QAction::triggered, this, &MainWindow::onSelectAll);

	formatAction = new QAction(tr("&Format Card..."), this);
	formatAction->setStatusTip(tr("Format the memory card (erase all data)"));
	formatAction->setEnabled(false);
	connect(formatAction, &QAction::triggered, this, &MainWindow::onFormatCard);

	eccToolAction = new QAction(tr("Remove &ECC and Save As..."), this);
	eccToolAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
	eccToolAction->setStatusTip(tr("Add or remove ECC from the memory card and save to a new file"));
	eccToolAction->setEnabled(false);
	connect(eccToolAction, &QAction::triggered, this, &MainWindow::onEccTool);

	asciiAction = new QAction(tr("&ASCII Descriptions"), this);
	asciiAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A));
	asciiAction->setStatusTip(tr("Show descriptions in ASCII instead of Shift-JIS"));
	asciiAction->setCheckable(true);
	asciiAction->setChecked(m_config ? m_config->getAsciiMode() : false);
	connect(asciiAction, &QAction::triggered, this, &MainWindow::onToggleAscii);

	forceImportAction = new QAction(tr("&Force Import"), this);
	forceImportAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
	forceImportAction->setStatusTip(tr("Force overwriting existing saves when importing"));
	forceImportAction->setCheckable(true);
	forceImportAction->setChecked(m_config ? m_config->getForceImport() : false);
	connect(forceImportAction, &QAction::triggered, this, &MainWindow::onToggleForceImport);

	preferencesAction = new QAction(tr("&Preferences..."), this);
	preferencesAction->setStatusTip(tr("Configure application settings"));
	connect(preferencesAction, &QAction::triggered, this, &MainWindow::onPreferences);

	aboutAction = new QAction(tr("&About"), this);
	aboutAction->setStatusTip(tr("Show information about myMCpp"));
	aboutAction->setIcon(QIcon(":/icons/AppIcon.png"));
	connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);

	gitHubAction = new QAction(tr("&GitHub Repository..."), this);
	gitHubAction->setStatusTip(tr("Visit myMCpp on GitHub"));
	gitHubAction->setIcon(QIcon(":/icons/feather/github.svg"));
	connect(gitHubAction, &QAction::triggered, this, &MainWindow::onGitHubRepository);

	documentationAction = new QAction(tr("&Documentation..."), this);
	documentationAction->setStatusTip(tr("View online documentation"));
	documentationAction->setIcon(QIcon(":/icons/feather/book-open.svg"));
	connect(documentationAction, &QAction::triggered, this, &MainWindow::onDocumentation);

	discordAction = new QAction(tr("&Discord Server..."), this);
	discordAction->setStatusTip(tr("Join the Discord community"));
	discordAction->setIcon(QIcon(":/icons/discord.svg"));
	connect(discordAction, &QAction::triggered, this, &MainWindow::onDiscordServer);

	checkUpdatesAction = new QAction(tr("&Check for Updates..."), this);
	checkUpdatesAction->setStatusTip(tr("Check if a new version is available"));
	checkUpdatesAction->setIcon(QIcon(":/icons/feather/refresh-cw.svg"));
	connect(checkUpdatesAction, &QAction::triggered, this, &MainWindow::onCheckForUpdates);

	aboutQtAction = new QAction(tr("About &Qt..."), this);
	aboutQtAction->setStatusTip(tr("Show information about Qt"));
	aboutQtAction->setIcon(QIcon(":/icons/qt.svg"));
	connect(aboutQtAction, &QAction::triggered, this, &MainWindow::onAboutQt);
}

void MainWindow::createMenus()
{
	QMenuBar* menuBar = new QMenuBar();
	setMenuBar(menuBar);

	QMenu* fileMenu = menuBar->addMenu(tr("&File"));
	fileMenu->addAction(openAction);
	fileMenu->addAction(createAction);
	fileMenu->addAction(closeAction);
	fileMenu->addSeparator();
	fileMenu->addAction(saveAsAction);
	fileMenu->addAction(eccToolAction);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAction);

	QMenu* editMenu = menuBar->addMenu(tr("&Edit"));
	editMenu->addAction(selectAllAction);
	editMenu->addSeparator();
	editMenu->addAction(importAction);
	editMenu->addAction(exportAction);
	editMenu->addSeparator();
	editMenu->addAction(deleteAction);
	editMenu->addAction(formatAction);

	QMenu* optionsMenu = menuBar->addMenu(tr("&Options"));
	optionsMenu->addAction(asciiAction);
	optionsMenu->addAction(forceImportAction);
	optionsMenu->addSeparator();
	optionsMenu->addAction(preferencesAction);

	QMenu* helpMenu = menuBar->addMenu(tr("&Help"));
	helpMenu->addAction(documentationAction);
	helpMenu->addSeparator();
	helpMenu->addAction(gitHubAction);
	helpMenu->addAction(discordAction);
	helpMenu->addSeparator();
	helpMenu->addAction(checkUpdatesAction);
	helpMenu->addSeparator();
	helpMenu->addAction(aboutAction);
	helpMenu->addAction(aboutQtAction);
}

void MainWindow::createToolbar()
{
	QToolBar* toolbar = addToolBar(tr("Main Toolbar"));
	toolbar->addAction(openAction);
	toolbar->addAction(createAction);
	toolbar->addSeparator();
	toolbar->addAction(importAction);
	toolbar->addAction(exportAction);
	toolbar->addSeparator();
	toolbar->addAction(deleteAction);
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

		closeAction->setEnabled(true);
		saveAsAction->setEnabled(true);
		eccToolAction->setEnabled(true);
		importAction->setEnabled(true);
		exportAction->setEnabled(true);
		formatAction->setEnabled(true);
		selectAllAction->setEnabled(true);
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

		closeAction->setEnabled(true);
		saveAsAction->setEnabled(true);
		eccToolAction->setEnabled(true);
		importAction->setEnabled(true);
		exportAction->setEnabled(true);
		formatAction->setEnabled(true);
		selectAllAction->setEnabled(true);

		statusBar->showMessage(tr("Created %1 MB memory card").arg(sizeMB), 5000);
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
	if (!memoryCard || !cardBrowser->hasSaveSelected())
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

	QString savePath = cardBrowser->getCurrentSavePath();
	actionHandler->exportSave(memoryCard.get(), savePath, filename);
}

void MainWindow::onDeleteFile()
{
	if (!memoryCard || !cardBrowser->hasSaveSelected())
	{
		QMessageBox::information(this, tr("Delete"),
			tr("Please select a save to delete"));
		return;
	}

	QString savePath = cardBrowser->getCurrentSavePath();

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
	statusBar->showMessage(tr("Deleted %1").arg(savePath), 5000);

	updateCardView();
	detailsPanel->clear();
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

void MainWindow::onPreferences()
{
	SettingsDialog dialog(m_config, this);
	if (dialog.exec() == QDialog::Accepted)
	{
		if (detailsPanel)
		{
			detailsPanel->refreshConfig();
		}
	}
}

void MainWindow::onAbout()
{
	QPixmap icon(":/icons/AppIcon.png");
	icon = icon.scaledToWidth(64, Qt::SmoothTransformation);

	QMessageBox msgBox(this);
	msgBox.setWindowTitle(tr("About myMCpp"));
	msgBox.setIconPixmap(icon);
	msgBox.setText(
		"<h2>myMCpp v1.0.0</h2>"
		"<p><b>A Modern PS2 Memory Card Manager</b></p>"
		"<p>myMCpp is a utility for manipulating PlayStation 2 memory card images."
		" This is a C++ rewrite of the original mymc++ utility.</p>"
		"<p><b>Features:</b></p>"
		"<ul>"
		"<li>lorem ipsum dolor sit amet</li>"
		"</ul>"
		"<p><b>License:</b> GPLv3</p>"
		"<p><b>Original Project:</b> mymc by Ross Ridge (Public Domain)</p>");
	msgBox.exec();
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
	QDesktopServices::openUrl(QUrl("https://discord.gg/TBD"));
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
	if (!memoryCard || !cardBrowser->hasSaveSelected())
	{
		detailsPanel->clear();
		deleteAction->setEnabled(false);
		exportAction->setEnabled(false);
		return;
	}

	QTreeWidgetItem* item = cardBrowser->currentItem();
	if (!item)
	{
		return;
	}

	QString savePath = cardBrowser->getCurrentSavePath();
	QString size = item->text(1);
	QString modified = item->text(2);

	detailsPanel->setSave(memoryCard.get(), savePath, size, modified);

	deleteAction->setEnabled(true);
	exportAction->setEnabled(true);
}

void MainWindow::onCardItemDoubleClicked()
{
	if (cardBrowser->hasSaveSelected())
	{
		onExportSave();
	}
}

void MainWindow::updateCardView()
{
	cardBrowser->loadCard(memoryCard.get());
	updateStatusBar();
}

void MainWindow::updateStatusBar()
{
	if (memoryCard)
	{
		try
		{
			uint32_t freeSpace = memoryCard->getFreeSpace();
			double freeMB = freeSpace / (1024.0 * 1024.0);
			statusBar->showMessage(tr("Free space: %1 MB").arg(freeMB, 0, 'f', 2));
		}
		catch (...)
		{
			statusBar->showMessage(tr("Memory card open"));
		}
	}
	else
	{
		statusBar->showMessage(tr("No memory card open"));
	}
}

void MainWindow::closeCard()
{
	memoryCard.reset();
	currentCardPath.clear();

	cardBrowser->clear();
	detailsPanel->clear();

	closeAction->setEnabled(false);
	saveAsAction->setEnabled(false);
	eccToolAction->setEnabled(false);
	importAction->setEnabled(false);
	exportAction->setEnabled(false);
	formatAction->setEnabled(false);
	deleteAction->setEnabled(false);
	selectAllAction->setEnabled(false);

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
		statusBar->showMessage(tr("Saved to %1").arg(filename), 5000);
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

	cardBrowser->selectAll();
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
		statusBar->showMessage(tr("Saved to %1").arg(filename), 5000);
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
		asciiAction->setChecked(newValue);
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
		forceImportAction->setChecked(newValue);
		m_config->save();
		updateForceImportWarning();
	}
}

void MainWindow::updateForceImportWarning()
{
	if (m_config && m_config->getForceImport())
	{
		statusBar->setStyleSheet("QStatusBar { color: red; }");
	}
	else
	{
		statusBar->setStyleSheet("");
	}
	updateStatusBar();
}
