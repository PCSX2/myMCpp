// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QDialog>

namespace Ui
{
	class ExportSavesDialog;
}

class ExportSavesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ExportSavesDialog(const QStringList& savePaths, QWidget* parent = nullptr);
	~ExportSavesDialog();
	QStringList getFilenames() const;

protected:
	void changeEvent(QEvent* event) override;
	void accept() override;

private:
	void retranslateFormatCombo();
	std::unique_ptr<Ui::ExportSavesDialog> ui;
};
