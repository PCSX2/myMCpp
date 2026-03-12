// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "SettingsWidget.h"

class QCheckBox;
class QPushButton;

class SettingsWindow;
namespace Ui
{
	class AdvancedSettingsWidget;
}

class AdvancedSettingsWidget : public SettingsWidget
{
	Q_OBJECT

public:
	explicit AdvancedSettingsWidget(SettingsWindow* dialog, QWidget* parent = nullptr);
	~AdvancedSettingsWidget();

	void saveSettings() override;
	void loadSettings() override;
	void restoreDefaults() override;

private slots:
	void onExportSettings();
	void onImportSettings();

protected:
	void changeEvent(QEvent* event) override;

private:
	std::unique_ptr<Ui::AdvancedSettingsWidget> ui;
};
