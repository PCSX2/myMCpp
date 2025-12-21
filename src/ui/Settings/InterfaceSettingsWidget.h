// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "SettingsWidget.h"
#include <memory>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QSpinBox;
class SettingsWindow;
namespace Ui { class InterfaceSettingsWidget; } 

class InterfaceSettingsWidget : public SettingsWidget
{
	Q_OBJECT

public:
	explicit InterfaceSettingsWidget(SettingsWindow* dialog, QWidget* parent = nullptr);
	~InterfaceSettingsWidget();

	void saveSettings() override;
	void loadSettings() override;
	void restoreDefaults() override;

private:
	std::unique_ptr<Ui::InterfaceSettingsWidget> ui;
};
