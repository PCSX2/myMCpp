// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "NewCardDialog.h"
#include <QButtonGroup>
#include <QIcon>
#include <QPushButton>

NewCardDialog::NewCardDialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::NewCardDialog)
{
	ui->setupUi(this);
	ui->introIconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("memcard-line")).pixmap(32, 32));

	sizeGroup = new QButtonGroup(this);
	sizeGroup->addButton(ui->size8MB, 8);
	sizeGroup->addButton(ui->size16MB, 16);
	sizeGroup->addButton(ui->size32MB, 32);
	sizeGroup->addButton(ui->size64MB, 64);

	connect(sizeGroup, &QButtonGroup::idClicked, this, [this](int) {
		updateSelectionDetails();
	});
	connect(ui->disableEccCheckbox, &QCheckBox::toggled, this, [this](bool) {
		updateSelectionDetails();
	});

	if (QPushButton* okButton = ui->buttonBox->button(QDialogButtonBox::Ok))
	{
		okButton->setText(tr("Continue..."));
	}

	updateSelectionDetails();
}

NewCardDialog::~NewCardDialog()
{
}

void NewCardDialog::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		if (ui)
		{
			ui->retranslateUi(this);
			updateSelectionDetails();
		}
	}
	QDialog::changeEvent(event);
}

int NewCardDialog::getCardSizeMB() const
{
	return sizeGroup->checkedId();
}

bool NewCardDialog::getDisableEcc() const
{
	return ui->disableEccCheckbox->isChecked();
}

void NewCardDialog::updateSelectionDetails()
{
	const int sizeMB = getCardSizeMB();
	const bool disableEcc = getDisableEcc();

	QString sizeText;
	if (sizeMB == 8)
		sizeText = tr("Best compatibility");
	else if (sizeMB == 16 || sizeMB == 32)
		sizeText = tr("Good compatibility with most titles");
	else
		sizeText = tr("Lower compatibility in some games");

	const QString eccText = disableEcc
		? tr("ECC disabled (smaller raw image)")
		: tr("ECC enabled (recommended default)");

	ui->selectionSummaryLabel->setText(
		tr("Current selection: %1 MB\n%2\n%3")
			.arg(sizeMB)
			.arg(sizeText)
			.arg(eccText));
}
