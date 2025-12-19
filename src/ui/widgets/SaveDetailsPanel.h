// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QWidget>
#include <QLabel>
#include <memory>

class PS2MemoryCard;
class IconWidget;
class Config;
class QVBoxLayout;

#include "ui_SaveDetailsPanel.h"

class SaveDetailsPanel : public QWidget
{
	Q_OBJECT

public:
	explicit SaveDetailsPanel(QWidget* parent = nullptr);

	void setConfig(Config* config);

	void setSave(PS2MemoryCard* card, const QString& savePath,
		const QString& size, const QString& modified);
	void clear();
	void refreshConfig();

private:
	void createIconWidget();

	Config* m_config;
	IconWidget* iconWidget;

	PS2MemoryCard* currentCard = nullptr;
	QString currentSavePath;
	QString currentSize;
	QString currentModified;

	std::string m_lastRendererType;

	std::unique_ptr<Ui::SaveDetailsPanel> ui;
};
