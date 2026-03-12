// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "SettingsWidget.h"
#include <memory>

class QCheckBox;
class QComboBox;
class QSpinBox;
class SettingsWindow;
namespace Ui
{
	class GeneralSettingsWidget;
}

class GeneralSettingsWidget : public SettingsWidget
{
	Q_OBJECT

public:
	explicit GeneralSettingsWidget(SettingsWindow* dialog, QWidget* parent = nullptr);
	~GeneralSettingsWidget();

	void saveSettings() override;
	void loadSettings() override;
	void restoreDefaults() override;

protected:
	void changeEvent(QEvent* event) override;

private:
	std::unique_ptr<Ui::GeneralSettingsWidget> ui;
	QWidget* m_rootWidget = nullptr;
};
