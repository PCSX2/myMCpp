// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <QDialog>
#include <memory>

namespace Ui
{
	class AboutDialog;
}

class AboutDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AboutDialog(QWidget* parent = nullptr);
	~AboutDialog();

protected:
	void changeEvent(QEvent* event) override;

private:
	std::unique_ptr<Ui::AboutDialog> ui;
};
