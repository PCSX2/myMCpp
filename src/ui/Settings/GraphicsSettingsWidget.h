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
	class GraphicsSettingsWidget;
}

class GraphicsSettingsWidget : public SettingsWidget
{
	Q_OBJECT

public:
	explicit GraphicsSettingsWidget(SettingsWindow* dialog, QWidget* parent = nullptr);
	~GraphicsSettingsWidget();

	void saveSettings() override;
	void loadSettings() override;
	void restoreDefaults() override;

private slots:
	void onRendererChanged(int index);
	void onVSyncChanged(bool enabled);

protected:
	void changeEvent(QEvent* event) override;

private:
	std::unique_ptr<Ui::GraphicsSettingsWidget> ui;
	QWidget* m_rootWidget = nullptr;
};
