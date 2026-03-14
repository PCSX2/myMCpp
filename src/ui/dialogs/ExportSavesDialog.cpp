// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "ExportSavesDialog.h"
#include "ui_ExportSavesDialog.h"
#include <QFileInfo>
#include <QMessageBox>

static const char* const PSU_EXT = ".psu";
static const char* const MAX_EXT = ".max";

ExportSavesDialog::ExportSavesDialog(const QStringList& savePaths, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::ExportSavesDialog)
{
	ui->setupUi(this);
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
					base = ui->tableWidget->item(i, 0)->text();
				item->setText(base + ext);
			}
		}
	});

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

ExportSavesDialog::~ExportSavesDialog() = default;

QStringList ExportSavesDialog::getFilenames() const
{
	QString ext = ui->formatCombo->currentData().toString();
	QStringList out;
	for (int i = 0; i < ui->tableWidget->rowCount(); ++i)
	{
		QString name = ui->tableWidget->item(i, 1)->text().trimmed();
		if (name.isEmpty())
			name = ui->tableWidget->item(i, 0)->text() + ext;
		else if (!name.contains(QLatin1Char('.')))
			name += ext;
		out.append(QFileInfo(name).fileName());
	}
	return out;
}

void ExportSavesDialog::accept()
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
	QDialog::accept();
}

void ExportSavesDialog::retranslateFormatCombo()
{
	int idx = ui->formatCombo->currentIndex();
	ui->formatCombo->clear();
	ui->formatCombo->addItem(tr("PSU (.psu)"), QVariant(QLatin1String(PSU_EXT)));
	ui->formatCombo->addItem(tr("MAX (.max)"), QVariant(QLatin1String(MAX_EXT)));
	ui->formatCombo->setCurrentIndex(idx == 1 ? 1 : 0);
}

void ExportSavesDialog::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange && ui)
	{
		ui->retranslateUi(this);
		retranslateFormatCombo();
	}
	QDialog::changeEvent(event);
}
