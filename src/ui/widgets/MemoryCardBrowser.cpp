// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MemoryCardBrowser.h"
#include "ps2mc.h"
#include "TranslationManager.h"
#include <QMessageBox>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QHeaderView>

MemoryCardBrowser::MemoryCardBrowser(QWidget* parent)
	: QTreeWidget(parent)
	, ui(new Ui::MemoryCardBrowser)
{
	ui->setupUi(this);
	connect(&TranslationManager::instance(), &TranslationManager::languageChanged, this, [this]() {
		ui->retranslateUi(this);
	});
}

void MemoryCardBrowser::clear()
{
	QTreeWidget::clear();
}

void MemoryCardBrowser::loadCard(PS2MemoryCard* card)
{
	clear();

	if (!card)
	{
		return;
	}

	try
	{
		auto entries = card->listDir("/");

		for (const auto& entry : entries)
		{
			if (entry.mode & DF_DIR && !(entry.mode & DF_HIDDEN))
			{
				QTreeWidgetItem* item = new QTreeWidgetItem(this);

				QString savePath = "/" + QString::fromStdString(entry.name);

				QString displayName;
				try
				{
					std::string title = card->getSaveTitle(savePath.toStdString());
					std::string subtitle = card->getSaveSubtitle(savePath.toStdString());

					if (!title.empty())
					{
						displayName = QString::fromStdString(title);
						if (!subtitle.empty())
						{
							displayName += " " + QString::fromStdString(subtitle);
						}
					}
				}
				catch (...)
				{
				}

				if (displayName.isEmpty())
				{
					displayName = QString::fromStdString(entry.name);
				}

				item->setText(0, displayName);

				uint32_t saveSize = card->getSaveSize(savePath.toStdString());
				double sizeKB = saveSize / 1024.0;
				item->setText(1, QString("%1 KB").arg(static_cast<int>(sizeKB)));

				item->setData(0, Qt::UserRole, QString::fromStdString(entry.name));
				auto time = todToTime(entry.modified);
				QDateTime dateTime = QDateTime::fromSecsSinceEpoch(time);
				item->setText(2, dateTime.toString("yyyy-MM-dd hh:mm"));

				addTopLevelItem(item);
			}
		}

		expandAll();
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(nullptr, "Error",
			QString("Failed to read memory card: %1").arg(e.what()));
	}
}

QString MemoryCardBrowser::getCurrentSavePath() const
{
	QTreeWidgetItem* item = currentItem();
	if (!item)
	{
		return QString();
	}

	QString path = item->data(0, Qt::UserRole).toString();
	QTreeWidgetItem* parent = item->parent();
	while (parent)
	{
		path = parent->data(0, Qt::UserRole).toString() + "/" + path;
		parent = parent->parent();
	}

	return "/" + path;
}

bool MemoryCardBrowser::hasSaveSelected() const
{
	QTreeWidgetItem* item = currentItem();
	return item != nullptr;
}

void MemoryCardBrowser::selectAll()
{
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		topLevelItem(i)->setSelected(true);
	}
}

void MemoryCardBrowser::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasUrls())
	{
		QList<QUrl> urls = event->mimeData()->urls();
		for (const QUrl& url : urls)
		{
			QString file = url.toLocalFile();
			if (file.endsWith(".psu", Qt::CaseInsensitive) ||
				file.endsWith(".max", Qt::CaseInsensitive) ||
				file.endsWith(".sps", Qt::CaseInsensitive) ||
				file.endsWith(".cbs", Qt::CaseInsensitive) ||
				file.endsWith(".xps", Qt::CaseInsensitive))
			{
				event->acceptProposedAction();
				return;
			}
		}
	}
}

void MemoryCardBrowser::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData->hasUrls())
	{
		QList<QUrl> urls = mimeData->urls();
		if (!urls.isEmpty())
		{
			QString file = urls.first().toLocalFile();
			emit saveFileDropped(file);
		}
	}
}
