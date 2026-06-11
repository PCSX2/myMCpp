// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QDialog>
#include <QStringList>
#include <QList>
#include <memory>

class PS2SaveFile;

namespace Ui
{
	class ImportExportSavesDialog;
}

class ImportExportSavesDialog : public QDialog
{
	Q_OBJECT

public:
	enum class Mode
	{
		Import,
		Export
	};

	struct ImportItem
	{
		QString filePath;
		QString gameTitle;
		QString directoryId;
		bool exists = false;
		bool corrupt = false;
		QString errorText;
		std::shared_ptr<PS2SaveFile> saveFile;

		// Output state from the dialog
		bool importSelected = true;
		bool overwriteSelected = false;
	};

	// Export Mode constructor
	explicit ImportExportSavesDialog(const QStringList& savePaths, QWidget* parent = nullptr);

	// Import Mode constructor
	explicit ImportExportSavesDialog(QList<ImportItem>& importItems, QWidget* parent = nullptr);

	~ImportExportSavesDialog();

	// Export Mode output
	QStringList getFilenames() const;

protected:
	void changeEvent(QEvent* event) override;
	void accept() override;

private:
	void retranslateFormatCombo();

	Mode m_mode;
	QList<ImportItem>* m_importItems = nullptr;

	std::unique_ptr<Ui::ImportExportSavesDialog> ui;
};
