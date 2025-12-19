// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QMainWindow>
#include <memory>

class PS2MemoryCard;
class MemoryCardBrowser;
class SaveDetailsPanel;
class CardActionHandler;
class QStatusBar;
class QLabel;
class Config;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(Config* config, QWidget* parent = nullptr);
	~MainWindow();

private slots:
	void onOpenMemoryCard();
	void onCreateMemoryCard();
	void onCloseMemoryCard();
	void onSaveAs();
	void onImportSave();
	void onExportSave();
	void onDeleteFile();
	void onSelectAll();
	void onFormatCard();
	void onEccTool();
	void onToggleAscii();
	void onToggleForceImport();
	void onPreferences();
	void onAbout();
	void onGitHubRepository();
	void onDocumentation();
	void onDiscordServer();
	void onCheckForUpdates();
	void onAboutQt();
	void onCardItemSelected();
	void onCardItemDoubleClicked();

private:
	void setupUI();
	void createActions();
	void createMenus();
	void createToolbar();
	void updateCardView();
	void updateStatusBar();
	void updateForceImportWarning();
	void closeCard();

	MemoryCardBrowser* cardBrowser;
	SaveDetailsPanel* detailsPanel;
	QStatusBar* statusBar;
	QLabel* forceImportWarning;

	QAction* openAction;
	QAction* createAction;
	QAction* closeAction;
	QAction* saveAsAction;
	QAction* exitAction;
	QAction* importAction;
	QAction* exportAction;
	QAction* deleteAction;
	QAction* selectAllAction;
	QAction* formatAction;
	QAction* eccToolAction;
	QAction* asciiAction;
	QAction* forceImportAction;
	QAction* preferencesAction;
	QAction* aboutAction;
	QAction* gitHubAction;
	QAction* documentationAction;
	QAction* discordAction;
	QAction* checkUpdatesAction;
	QAction* aboutQtAction;

	std::unique_ptr<CardActionHandler> actionHandler;
	std::unique_ptr<PS2MemoryCard> memoryCard;
	QString currentCardPath;

	Config* m_config;
};
