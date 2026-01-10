// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QTreeWidget>
#include <QDateTime>
#include <QString>
#include <memory>

class PS2MemoryCard;

#include "ui_MemoryCardBrowser.h"

class MemoryCardBrowser : public QTreeWidget
{
	Q_OBJECT

public:
	explicit MemoryCardBrowser(QWidget* parent = nullptr);

	void loadCard(PS2MemoryCard* card);
	void clear();
	void selectAll() override;

	QString getCurrentSavePath() const;
	QString getSelectedPath() const;
	bool hasSaveSelected() const;

	void navigateTo(const QString& path);
	void navigateUp();
	bool isInsideSave() const;
	QString getCurrentPath() const { return m_currentPath; }

signals:
	void saveFileDropped(const QString& filePath);
	void pathChanged(const QString& newPath);
	void exportSaveRequested(const QString& savePath);
	void exportFileRequested(const QString& savePath, const QString& fileName);
	void deleteSaveRequested(const QString& savePath);
	void importFileRequested(const QString& savePath, const QString& hostFilePath);
	void createFolderRequested(const QString& parentPath);
	void importArbitraryFileRequested(const QString& targetDir);

protected:
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;
	void contextMenuEvent(QContextMenuEvent* event) override;

private:
	void loadRootDirectory();
	void loadSaveDirectory(const QString& savePath);

	std::unique_ptr<Ui::MemoryCardBrowser> ui;
	PS2MemoryCard* m_card = nullptr;
	QString m_currentPath = "/";
};
