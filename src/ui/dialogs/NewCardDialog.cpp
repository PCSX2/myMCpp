// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "NewCardDialog.h"
#include "core/formats/PS2MemoryCard.h"
#include <QIcon>
#include <QPushButton>
#include <QSignalBlocker>

NewCardDialog::NewCardDialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::NewCardDialog)
{
	ui->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->iconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("memcard-line")).pixmap(ui->iconLabel->size()));

	ui->sizeButtonGroup->setId(ui->size8MB, 8);
	ui->sizeButtonGroup->setId(ui->size16MB, 16);
	ui->sizeButtonGroup->setId(ui->size32MB, 32);
	ui->sizeButtonGroup->setId(ui->size64MB, 64);

	connect(ui->nameEdit, &QLineEdit::textChanged, this, &NewCardDialog::nameTextChanged);

	retranslateFormatCombo();
	if (QPushButton* okButton = ui->buttonBox->button(QDialogButtonBox::Ok))
		okButton->setText(tr("Continue..."));

	ui->nameEdit->setFocus();
	updateState();
}

NewCardDialog::~NewCardDialog() = default;

void NewCardDialog::retranslateFormatCombo()
{
	const QString ext = ui->formatCombo->currentData().toString();
	ui->formatCombo->clear();
	for (const PS2MemoryCard::CardFormat& format : PS2MemoryCard::getFormats())
		ui->formatCombo->addItem(QCoreApplication::translate("CardFormats", format.filterName), QLatin1String(format.extension));

	const int idx = ui->formatCombo->findData(ext);
	if (idx >= 0)
		ui->formatCombo->setCurrentIndex(idx);
}

void NewCardDialog::nameTextChanged()
{
	QString name = ui->nameEdit->text();
	const int cursorPos = ui->nameEdit->cursorPosition();
	name.remove(QLatin1Char('.'));

	QSignalBlocker sb(ui->nameEdit);
	ui->nameEdit->setText(name);
	ui->nameEdit->setCursorPosition(cursorPos);

	updateState();
}

void NewCardDialog::updateState()
{
	if (QPushButton* okButton = ui->buttonBox->button(QDialogButtonBox::Ok))
		okButton->setEnabled(!ui->nameEdit->text().trimmed().isEmpty());
}

void NewCardDialog::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange && ui)
	{
		ui->retranslateUi(this);
		retranslateFormatCombo();
		if (QPushButton* okButton = ui->buttonBox->button(QDialogButtonBox::Ok))
			okButton->setText(tr("Continue..."));
		updateState();
	}
	QDialog::changeEvent(event);
}

int NewCardDialog::getCardSizeMB() const
{
	return ui->sizeButtonGroup->checkedId();
}

QString NewCardDialog::getCardName() const
{
	return ui->nameEdit->text().trimmed();
}

QString NewCardDialog::getCardExtension() const
{
	return ui->formatCombo->currentData().toString();
}

bool NewCardDialog::usesEcc() const
{
	const QString ext = getCardExtension();
	for (const PS2MemoryCard::CardFormat& format : PS2MemoryCard::getFormats())
	{
		if (ext == QLatin1String(format.extension))
			return format.usesEcc;
	}
	return true;
}

QString NewCardDialog::getFileName() const
{
	return getCardName() + QLatin1Char('.') + getCardExtension();
}
