// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QDialog>
#include <memory>
#include <vector>

class Config;
class QListWidget;
class QStackedWidget;
class QLabel;
class QTimer;
namespace Ui { class SettingsWindow; }

class SettingsWidget;

class SettingsWindow : public QDialog
{
	Q_OBJECT

public:
	explicit SettingsWindow(Config* config, QWidget* parent = nullptr);
	~SettingsWindow();

	Config* getConfig() const { return m_config; }

	void registerWidgetHelp(QObject* widget, const QString& title, const QString& description);

signals:
	void applicationSettingsChanged();

private slots:
	void onCategorySelected(int row);
	void onRestoreDefaults();
	void onClose();
	void onSettingChanged();
	void saveToDisk();

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	void updateDescription(int index);
	void addSettingsWidget(SettingsWidget* widget, const QString& name, const QIcon& icon, const QString& description);

	Config* m_config;
	std::unique_ptr<Ui::SettingsWindow> ui;

	std::vector<SettingsWidget*> m_settingsWidgets;
	std::vector<QString> m_descriptions;

	struct HelpEntry {
		QString title;
		QString description;
	};
	QMap<QObject*, HelpEntry> m_helpMap;
	QString m_currentCategoryDescription;

	QTimer* m_saveTimer;
};
