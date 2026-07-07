// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

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

signals:
	void iconWidgetChanged(IconWidget* widget);

private:
	void createIconWidget();
	void updatePlayPauseButton();

	Config* m_config;
	IconWidget* iconWidget;

	PS2MemoryCard* currentCard = nullptr;
	QString currentSavePath;
	QString currentSize;
	QString currentModified;

	std::string m_lastRendererType;
	std::string m_lastAdapter;

	std::unique_ptr<Ui::SaveDetailsPanel> ui;
};
