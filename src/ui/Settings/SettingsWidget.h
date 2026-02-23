// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QWidget>

class QScrollArea;
class QTabWidget;
class SettingsWindow;
class QVBoxLayout;

class SettingsWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SettingsWidget(SettingsWindow* dialog, QWidget* parent = nullptr);
	virtual ~SettingsWidget() = default;

	// Adds a header widget to the top of the page (above tabs).
	void addPageHeader(QWidget* header, bool custom_margins = false);

	// Adds a new tab with the given name and contents.
	// Returns the QScrollArea wrapping the contents.
	QWidget* addTab(const QString& name, QWidget* contents, bool custom_margins = false);

	// Saves the settings handled by this widget.
	virtual void saveSettings() {}
	
	// Reloads settings from config to UI.
	virtual void loadSettings() {}
	
	// Restores default values for this widget.
	virtual void restoreDefaults() {}

signals:
	void settingChanged();

protected:
	void registerHelp(QWidget* widget, const QString& title, const QString& description);

	SettingsWindow* m_dialog;
	QTabWidget* m_tab_widget;

private:
	void updateTabMargins(QScrollArea* scroll_area);
	QScrollArea* m_last_scroll_area = nullptr;
};
