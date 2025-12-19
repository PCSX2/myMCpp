// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QDialog>

class QRadioButton;
#include <memory>

class QButtonGroup;

#include "ui_NewCardDialog.h"

class NewCardDialog : public QDialog
{
	Q_OBJECT

public:
	explicit NewCardDialog(QWidget* parent = nullptr);

	~NewCardDialog();

	int getCardSizeMB() const;
	bool getDisableEcc() const;

private:
	std::unique_ptr<Ui::NewCardDialog> ui;
	QButtonGroup* sizeGroup;
};
