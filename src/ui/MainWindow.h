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

#include "ui_MainWindow.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(Config* config, QWidget* parent = nullptr);
	~MainWindow();

protected:
	void changeEvent(QEvent* event) override;

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
	void onSettingsChanged();

private:
	void updateCardView();
	void updateStatusBar();
	void updateForceImportWarning();
	void closeCard();

	// UI Structure
	std::unique_ptr<Ui::MainWindow> ui;

	std::unique_ptr<CardActionHandler> actionHandler;
	std::unique_ptr<PS2MemoryCard> memoryCard;
	QString currentCardPath;

	Config* m_config;
	class SettingsWindow* m_settingsWindow;
};
