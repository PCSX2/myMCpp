// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QDialog>

#include <memory>

#include "ui_NewCardDialog.h"

class NewCardDialog : public QDialog
{
	Q_OBJECT

public:
	explicit NewCardDialog(QWidget* parent = nullptr);

	~NewCardDialog();

	int getCardSizeMB() const;
	QString getCardName() const;
	QString getCardExtension() const;
	bool usesEcc() const;
	QString getFileName() const;

protected:
	void changeEvent(QEvent* event) override;

private:
	void retranslateFormatCombo();
	void nameTextChanged();
	void updateState();

	std::unique_ptr<Ui::NewCardDialog> ui;
};
