// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QDialog>

#include <memory>

#include "ui_NewCardDialog.h"

class QRadioButton;
class QButtonGroup;

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
