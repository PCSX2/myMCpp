// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QDialog>
#include <memory>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QLineEdit;
class Config;

#include "ui_SettingsDialog.h"

class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SettingsDialog(Config* config, QWidget* parent = nullptr);
	~SettingsDialog();

private slots:
	void onCategorySelected(QListWidgetItem* item);
	void onAccepted();
	void onBrowseMemoryCardPath();
	void onRendererChanged(const QString& renderer);

private:
	void loadCurrentSettings();

	Config* m_config;
	std::unique_ptr<Ui::SettingsDialog> ui;
};
