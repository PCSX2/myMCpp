// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QString>
#include <QObject>

class PS2MemoryCard;
class PS2SaveFile;
class QWidget;
class QStatusBar;

class CardActionHandler : public QObject
{
	Q_OBJECT

public:
	explicit CardActionHandler(QWidget* parent = nullptr);

	PS2MemoryCard* openCard(const QString& filename);
	PS2MemoryCard* createCard(const QString& filename, int sizeMB = 8, bool disableEcc = false);
	void closeCard();

	enum class ImportResult
	{
		Success,
		Skipped,
		Failed
	};

	ImportResult importSave(PS2MemoryCard* card, const std::shared_ptr<PS2SaveFile>& saveFile, const QString& filename, bool showDialogs = true, bool forceOverwrite = false);
	bool exportSave(PS2MemoryCard* card, const QString& savePath, const QString& filename, bool showSuccessDialog = true);
	void deleteSave(PS2MemoryCard* card, const QString& savePath);
	void formatCard(PS2MemoryCard* card, const QString& cardPath, int sizeMB = -1);

	void setStatusBar(QStatusBar* statusBar);

private:
	QWidget* parentWidget;
	QStatusBar* statusBar;
};
