// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "NewCardDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QLabel>

NewCardDialog::NewCardDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Create New Memory Card"));
	setModal(true);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	QGroupBox* sizeGroup = new QGroupBox(tr("Card Size"), this);
	QVBoxLayout* sizeLayout = new QVBoxLayout(sizeGroup);

	this->sizeGroup = new QButtonGroup(this);

	size8MB = new QRadioButton(tr("8 MB (Standard)"), this);
	size16MB = new QRadioButton(tr("16 MB"), this);
	size32MB = new QRadioButton(tr("32 MB"), this);
	size64MB = new QRadioButton(tr("64 MB"), this);

	size8MB->setChecked(true); // Default to 8MB

	this->sizeGroup->addButton(size8MB, 8);
	this->sizeGroup->addButton(size16MB, 16);
	this->sizeGroup->addButton(size32MB, 32);
	this->sizeGroup->addButton(size64MB, 64);

	sizeLayout->addWidget(size8MB);
	sizeLayout->addWidget(size16MB);
	sizeLayout->addWidget(size32MB);
	sizeLayout->addWidget(size64MB);

	mainLayout->addWidget(sizeGroup);

	QGroupBox* optionsGroup = new QGroupBox(tr("Options"), this);
	QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

	disableEccCheckbox = new QCheckBox(tr("Disable ECC (for raw images)"), this);
	disableEccCheckbox->setChecked(false);
	disableEccCheckbox->setToolTip(tr("Creates a memory card image without ECC data.\n"
									  "Some emulators require non-ECC images."));

	optionsLayout->addWidget(disableEccCheckbox);
	mainLayout->addWidget(optionsGroup);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	mainLayout->addWidget(buttonBox);

	setLayout(mainLayout);
	setMinimumWidth(250);
}

int NewCardDialog::getCardSizeMB() const
{
	return sizeGroup->checkedId();
}

bool NewCardDialog::getDisableEcc() const
{
	return disableEccCheckbox->isChecked();
}
