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

class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SettingsDialog(Config* config, QWidget* parent = nullptr);

private slots:
	void onCategorySelected(QListWidgetItem* item);
	void onAccepted();
	void onBrowseMemoryCardPath();
	void onRendererChanged(const QString& renderer);

private:
	void setupUI();
	void createInterfaceCategory();
	void createBehaviorCategory();
	void createFilesCategory();
	void createAdvancedCategory();
	void loadCurrentSettings();

	QListWidget* categoryList;
	QStackedWidget* settingsStack;

	QCheckBox* darkModeCheck;
	QSpinBox* thumbnailSizeSpinner;
	QComboBox* rendererCombo;
	QCheckBox* animateIconsCheck;
	QComboBox* lightingCombo;
	QComboBox* cameraCombo;

	QCheckBox* warnOnDeleteCheck;
	QCheckBox* confirmShutdownCheck;
	QCheckBox* hideToTrayCheck;
	QLineEdit* memoryCardPathEdit;

	QCheckBox* enableDebugLogCheck;

	Config* m_config;
};
