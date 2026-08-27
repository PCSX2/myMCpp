// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "ImportExportSavesDialog.h"
#include "ui_ImportExportSavesDialog.h"
#include "core/formats/PS2SaveFile.h"
#include <QFileInfo>
#include <QMessageBox>
#include <QHeaderView>

static const char* const PSU_EXT = ".psu";
static const char* const MAX_EXT = ".max";

ImportExportSavesDialog::ImportExportSavesDialog(const QStringList& savePaths, QWidget* parent)
	: QDialog(parent)
	, m_mode(Mode::Export)
	, ui(new Ui::ImportExportSavesDialog)
{
	ui->setupUi(this);
	setWindowTitle(tr("Export File Names"));
	ui->introLabel->setText(tr("Edit export filenames (folder name by default):"));

	ui->formatCombo->addItem(tr("PSU (.psu)"), QVariant(QLatin1String(PSU_EXT)));
	ui->formatCombo->addItem(tr("MAX (.max)"), QVariant(QLatin1String(MAX_EXT)));
	connect(ui->formatCombo, &QComboBox::currentIndexChanged, this, [this]() {
		QString ext = ui->formatCombo->currentData().toString();
		for (int i = 0; i < ui->tableWidget->rowCount(); ++i)
		{
			QTableWidgetItem* item = ui->tableWidget->item(i, 1);
			if (!item)
				continue;
			QString name = item->text();
			if (name.endsWith(QLatin1String(PSU_EXT), Qt::CaseInsensitive) || name.endsWith(QLatin1String(MAX_EXT), Qt::CaseInsensitive))
			{
				QString base = name.left(name.lastIndexOf(QLatin1Char('.')));
				if (base.isEmpty())
				{
					QTableWidgetItem* saveItem = ui->tableWidget->item(i, 0);
					base = saveItem ? saveItem->text() : QString();
				}
				item->setText(base + ext);
			}
		}
	});

	ui->tableWidget->setColumnCount(2);
	ui->tableWidget->setHorizontalHeaderLabels({tr("Save"), tr("Export filename")});
	ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
	ui->tableWidget->setRowCount(savePaths.size());
	QString ext = ui->formatCombo->currentData().toString();

	for (int i = 0; i < savePaths.size(); ++i)
	{
		QString path = savePaths[i];
		QString displayName = path;
		if (displayName.startsWith('/'))
			displayName.remove(0, 1);
		QString defaultName = QFileInfo(path).fileName() + ext;

		QTableWidgetItem* pathItem = new QTableWidgetItem(displayName);
		pathItem->setFlags(pathItem->flags() & ~Qt::ItemIsEditable);
		ui->tableWidget->setItem(i, 0, pathItem);

		QTableWidgetItem* nameItem = new QTableWidgetItem(defaultName);
		ui->tableWidget->setItem(i, 1, nameItem);
	}

	ui->tableWidget->resizeColumnsToContents();
}

ImportExportSavesDialog::ImportExportSavesDialog(QList<ImportItem>& importItems, QWidget* parent)
	: QDialog(parent)
	, m_mode(Mode::Import)
	, m_importItems(&importItems)
	, ui(new Ui::ImportExportSavesDialog)
{
	ui->setupUi(this);
	setWindowTitle(tr("Import Saves"));
	ui->introLabel->setText(tr(
		"Review and select saves to import.\n"
		"Imports are saved to the open memory card file."));

	// Hide the format selection layout since it is only for export
	ui->formatLabel->hide();
	ui->formatCombo->hide();

	ui->tableWidget->setColumnCount(4);
	ui->tableWidget->setHorizontalHeaderLabels({tr("Import? / Game Title"), tr("Directory ID"), tr("Source File"), tr("Overwrite?")});
	ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
	ui->tableWidget->setRowCount(importItems.size());

	for (int i = 0; i < importItems.size(); ++i)
	{
		const ImportItem& item = importItems[i];

		QTableWidgetItem* titleItem = new QTableWidgetItem(item.gameTitle);
		titleItem->setCheckState(item.importSelected && !item.corrupt ? Qt::Checked : Qt::Unchecked);
		titleItem->setFlags((titleItem->flags() & ~Qt::ItemIsEditable) | Qt::ItemIsUserCheckable);

		if (item.corrupt)
		{
			titleItem->setText(tr("[Corrupt] %1").arg(item.errorText));
			titleItem->setForeground(Qt::red);
			titleItem->setFlags(titleItem->flags() & ~Qt::ItemIsUserCheckable);
		}
		ui->tableWidget->setItem(i, 0, titleItem);

		QTableWidgetItem* dirItem = new QTableWidgetItem(item.directoryId);
		dirItem->setFlags(dirItem->flags() & ~Qt::ItemIsEditable);
		ui->tableWidget->setItem(i, 1, dirItem);

		QTableWidgetItem* fileItem = new QTableWidgetItem(QFileInfo(item.filePath).fileName());
		fileItem->setFlags(fileItem->flags() & ~Qt::ItemIsEditable);
		ui->tableWidget->setItem(i, 2, fileItem);

		QTableWidgetItem* overwriteItem = new QTableWidgetItem();
		overwriteItem->setFlags(overwriteItem->flags() & ~Qt::ItemIsEditable);
		if (item.exists)
		{
			overwriteItem->setText(tr("Yes"));
			overwriteItem->setCheckState(item.overwriteSelected ? Qt::Checked : Qt::Unchecked);
			overwriteItem->setFlags(overwriteItem->flags() | Qt::ItemIsUserCheckable);
		}
		else
		{
			overwriteItem->setText(tr("No (New)"));
		}
		ui->tableWidget->setItem(i, 3, overwriteItem);
	}

	ui->tableWidget->resizeColumnsToContents();
}

ImportExportSavesDialog::~ImportExportSavesDialog() = default;

QStringList ImportExportSavesDialog::getFilenames() const
{
	if (m_mode != Mode::Export)
		return {};

	QString ext = ui->formatCombo->currentData().toString();
	QStringList out;
	for (int i = 0; i < ui->tableWidget->rowCount(); ++i)
	{
		QTableWidgetItem* nameItem = ui->tableWidget->item(i, 1);
		QTableWidgetItem* saveItem = ui->tableWidget->item(i, 0);
		QString name = nameItem ? nameItem->text().trimmed() : QString();
		if (name.isEmpty())
		{
			name = (saveItem ? saveItem->text() : QString()) + ext;
		}
		else if (!name.contains(QLatin1Char('.')))
		{
			name += ext;
		}
		out.append(QFileInfo(name).fileName());
	}
	return out;
}

void ImportExportSavesDialog::accept()
{
	if (m_mode == Mode::Export)
	{
		QStringList names = getFilenames();
		QSet<QString> seen;
		QStringList dups;
		for (const QString& n : names)
		{
			if (seen.contains(n))
				dups.append(n);
			else
				seen.insert(n);
		}
		if (!dups.isEmpty())
		{
			QString list = dups.join(QStringLiteral(", "));
			QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Duplicate Filenames"),
				tr("Some filenames are used more than once. Later files will overwrite earlier ones:\n\n%1\n\nContinue?").arg(list),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
			if (reply != QMessageBox::Yes)
				return;
		}
	}
	else if (m_mode == Mode::Import && m_importItems)
	{
		for (int i = 0; i < ui->tableWidget->rowCount() && i < m_importItems->size(); ++i)
		{
			QTableWidgetItem* titleItem = ui->tableWidget->item(i, 0);
			QTableWidgetItem* overwriteItem = ui->tableWidget->item(i, 3);

			if (titleItem)
			{
				(*m_importItems)[i].importSelected = (titleItem->checkState() == Qt::Checked);
			}
			if (overwriteItem && (*m_importItems)[i].exists)
			{
				(*m_importItems)[i].overwriteSelected = (overwriteItem->checkState() == Qt::Checked);
			}
		}
	}

	QDialog::accept();
}

void ImportExportSavesDialog::retranslateFormatCombo()
{
	if (m_mode != Mode::Export)
		return;

	int idx = ui->formatCombo->currentIndex();
	ui->formatCombo->clear();
	ui->formatCombo->addItem(tr("PSU (.psu)"), QVariant(QLatin1String(PSU_EXT)));
	ui->formatCombo->addItem(tr("MAX (.max)"), QVariant(QLatin1String(MAX_EXT)));
	ui->formatCombo->setCurrentIndex(idx == 1 ? 1 : 0);
}

void ImportExportSavesDialog::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange && ui)
	{
		ui->retranslateUi(this);
		retranslateFormatCombo();
	}
	QDialog::changeEvent(event);
}
