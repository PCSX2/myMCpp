// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "SettingsWidget.h"

class SettingsWindow;
namespace Ui
{
	class FilesSettingsWidget;
}

class FilesSettingsWidget : public SettingsWidget
{
	Q_OBJECT

public:
	explicit FilesSettingsWidget(SettingsWindow* dialog, QWidget* parent = nullptr);
	~FilesSettingsWidget();

	void saveSettings() override;
	void loadSettings() override;
	void restoreDefaults() override;

private slots:
	void onBrowseMemoryCardPath();
	void onBrowseImportExportPath();

protected:
	void changeEvent(QEvent* event) override;

private:
	std::unique_ptr<Ui::FilesSettingsWidget> ui;
	QWidget* m_rootWidget = nullptr;
};
