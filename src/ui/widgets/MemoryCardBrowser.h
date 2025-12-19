// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QTreeWidget>
#include <QDateTime>
#include <QString>

class PS2MemoryCard;

class MemoryCardBrowser : public QTreeWidget
{
	Q_OBJECT

public:
	explicit MemoryCardBrowser(QWidget* parent = nullptr);

	void loadCard(PS2MemoryCard* card);
	void clear();
	void selectAll() override;

	QString getCurrentSavePath() const;
	bool hasSaveSelected() const;

signals:
	void saveFileDropped(const QString& filePath);

protected:
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

private:
	void setupUI();
};
