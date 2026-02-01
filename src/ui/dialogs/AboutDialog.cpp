// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include "ResourcePath.h"
#include <QDesktopServices>
#include <QTextBrowser>
#include <QDialogButtonBox>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    const int targetHeight = 96;
    QPixmap pixmap = ui->logoLabel->pixmap();
    if (!pixmap.isNull()) {
        if (pixmap.height() > targetHeight) {
             ui->logoLabel->setPixmap(pixmap.scaledToHeight(targetHeight, Qt::SmoothTransformation));
        }
    }

    adjustSize();
    setFixedSize(size());

    connect(ui->linksLabel, &QLabel::linkActivated, this, [this](const QString &link) {
        if (link == "#licenses") {
            auto path = ResourcePath::get() / "licenses.html";
            QDialog* licenseDialog = new QDialog(this);
            licenseDialog->setWindowTitle("Third-Party Licenses");
            licenseDialog->resize(800, 600);
            
            QVBoxLayout* layout = new QVBoxLayout(licenseDialog);
            QTextBrowser* textBrowser = new QTextBrowser(licenseDialog);
            textBrowser->setSearchPaths({QString::fromStdString(ResourcePath::get().string())});
            textBrowser->setSource(QUrl::fromLocalFile(QString::fromStdString(path.string())));
            textBrowser->setOpenExternalLinks(true);
            layout->addWidget(textBrowser);
            
            QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, licenseDialog);
            connect(buttonBox, &QDialogButtonBox::rejected, licenseDialog, &QDialog::accept);
            layout->addWidget(buttonBox);
            
            licenseDialog->exec();
            delete licenseDialog;
        } else {
            QDesktopServices::openUrl(QUrl(link));
        }
    });
}

AboutDialog::~AboutDialog()
{
}
