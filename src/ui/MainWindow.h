// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QMainWindow>
#include <memory>

#include "ui_MainWindow.h"

class PS2MemoryCard;
class MemoryCardBrowser;
class SaveDetailsPanel;
class CardActionHandler;
class QStatusBar;
class QLabel;
class Config;
class DiscordRPCManager;

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
	void onCardInfo();
	void onImportSave();
	void onExportSave();
	void onDeleteFile();
	void onSelectAll();
	void onFormatCard();
	void onEccTool();
	void onToggleAscii();
	void onToggleForceImport();
	void onToggleToolbarLock();
	void onPreferences();
	void onAbout();
	void onGitHubRepository();
	void onDocumentation();
	void onCheckForUpdates();
	void onAboutQt();
	void onCardItemSelected();
	void onCardItemDoubleClicked();
	void onSettingsChanged();
	void onEditTimestamp(const QString& mcPath);
	void onRenameEntry(const QString& mcPath);
	void onLanguageChanged();

private:
	void updateCardView();
	void updateStatusBar();
	void updateForceImportWarning();
	void closeCard();
	void importFileToCard(const QString& savePath, const QString& hostFilePath);
	void importSaveFiles(const QStringList& paths);

	std::unique_ptr<Ui::MainWindow> ui;

	std::unique_ptr<CardActionHandler> actionHandler;
	std::unique_ptr<PS2MemoryCard> memoryCard;
	QString currentCardPath;

	Config* m_config;
	class SettingsWindow* m_settingsWindow;
	std::unique_ptr<DiscordRPCManager> m_discordRpc;
};
