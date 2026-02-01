// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "NewCardDialog.h"
#include <QButtonGroup>

NewCardDialog::NewCardDialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::NewCardDialog)
{
	ui->setupUi(this);

	sizeGroup = new QButtonGroup(this);
	sizeGroup->addButton(ui->size8MB, 8);
	sizeGroup->addButton(ui->size16MB, 16);
	sizeGroup->addButton(ui->size32MB, 32);
	sizeGroup->addButton(ui->size64MB, 64);
}

NewCardDialog::~NewCardDialog()
{
}

int NewCardDialog::getCardSizeMB() const
{
	return sizeGroup->checkedId();
}

bool NewCardDialog::getDisableEcc() const
{
	return ui->disableEccCheckbox->isChecked();
}
