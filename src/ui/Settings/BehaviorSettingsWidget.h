// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "SettingsWidget.h"
#include <memory>

class QCheckBox;

class SettingsWindow;
namespace Ui { class BehaviorSettingsWidget; }

class BehaviorSettingsWidget : public SettingsWidget
{
	Q_OBJECT

public:
	explicit BehaviorSettingsWidget(SettingsWindow* dialog, QWidget* parent = nullptr);
	~BehaviorSettingsWidget();

	void saveSettings() override;
	void loadSettings() override;
	void restoreDefaults() override;

private:
	std::unique_ptr<Ui::BehaviorSettingsWidget> ui;
};
