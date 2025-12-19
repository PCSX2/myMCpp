// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QDialog>

class QRadioButton;
class QCheckBox;
class QButtonGroup;

class NewCardDialog : public QDialog
{
	Q_OBJECT

public:
	explicit NewCardDialog(QWidget* parent = nullptr);

	int getCardSizeMB() const;
	bool getDisableEcc() const;

private:
	QButtonGroup* sizeGroup;
	QRadioButton* size8MB;
	QRadioButton* size16MB;
	QRadioButton* size32MB;
	QRadioButton* size64MB;
	QCheckBox* disableEccCheckbox;
};
