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
#include <QMenu>
#include <QAction>

MemoryCardBrowser::MemoryCardBrowser(QWidget* parent)
	: QTreeWidget(parent)
	, ui(new Ui::MemoryCardBrowser)
{
	ui->setupUi(this);
	connect(&TranslationManager::instance(), &TranslationManager::languageChanged, this, [this]() {
		ui->retranslateUi(this);
	});

	header()->setSectionResizeMode(0, QHeaderView::Stretch);
	header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}

void MemoryCardBrowser::clear()
{
	QTreeWidget::clear();
	m_currentPath = "/";
}

void MemoryCardBrowser::loadCard(PS2MemoryCard* card)
{
	clear();
	m_card = card;
	m_currentPath = "/";

	if (!card)
	{
		return;
	}

	loadRootDirectory();
}

void MemoryCardBrowser::loadRootDirectory()
{
	QTreeWidget::clear();

	if (!m_card)
		return;

	try
	{
		auto entries = m_card->listDir("/");

		for (const auto& entry : entries)
		{
			if ((entry.mode & DF_EXISTS) && !(entry.mode & DF_HIDDEN))
			{
				if (entry.name == "." || entry.name == "..")
					continue;

				QTreeWidgetItem* item = new QTreeWidgetItem(this);
				QString savePath = "/" + QString::fromStdString(entry.name);
				QString displayName;

				bool isDir = (entry.mode & DF_DIR) != 0;

				if (isDir)
				{
					try
					{
						std::string title = m_card->getSaveTitle(savePath.toStdString());
						std::string subtitle = m_card->getSaveSubtitle(savePath.toStdString());

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
				}

				if (displayName.isEmpty())
				{
					displayName = QString::fromStdString(entry.name);
				}

				item->setText(0, displayName);

				if (isDir)
				{
					try {
						uint32_t saveSize = m_card->getSaveSize(savePath.toStdString());
						double sizeKB = saveSize / 1024.0;
						item->setText(1, QString("%1 KB").arg(static_cast<int>(sizeKB)));
					} catch(...) {
						item->setText(1, tr("?"));
					}
				}
				else
				{
					double sizeKB = entry.length / 1024.0;
					if (sizeKB < 1.0)
						item->setText(1, QString("%1 B").arg(entry.length));
					else
						item->setText(1, QString("%1 KB").arg(static_cast<int>(sizeKB)));
				}

				item->setData(0, Qt::UserRole, QString::fromStdString(entry.name));
				item->setData(0, Qt::UserRole + 1, isDir);
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
		QMessageBox::critical(nullptr, tr("Error"),
			tr("Failed to read memory card: %1").arg(e.what()));
	}
}

void MemoryCardBrowser::loadSaveDirectory(const QString& savePath)
{
	QTreeWidget::clear();

	if (!m_card)
		return;

	try
	{
		QTreeWidgetItem* parentItem = new QTreeWidgetItem(this);
		parentItem->setText(0, "..");
		parentItem->setText(1, "");
		parentItem->setText(2, "");
		parentItem->setData(0, Qt::UserRole, "..");
		parentItem->setData(0, Qt::UserRole + 1, true);
		addTopLevelItem(parentItem);

		auto entries = m_card->listDir(savePath.toStdString());

		for (const auto& entry : entries)
		{
			if (entry.name == "." || entry.name == "..")
				continue;

			QTreeWidgetItem* item = new QTreeWidgetItem(this);

			item->setText(0, QString::fromStdString(entry.name));

			bool isDir = (entry.mode & DF_DIR) != 0;
			item->setData(0, Qt::UserRole, QString::fromStdString(entry.name));
			item->setData(0, Qt::UserRole + 1, isDir);

			if (!isDir)
			{
				double sizeKB = entry.length / 1024.0;
				if (sizeKB < 1.0)
					item->setText(1, QString("%1 B").arg(entry.length));
				else
					item->setText(1, QString("%1 KB").arg(static_cast<int>(sizeKB)));
			}
			else
			{
				item->setText(1, tr("<DIR>"));
			}

			auto time = todToTime(entry.modified);
			QDateTime dateTime = QDateTime::fromSecsSinceEpoch(time);
			item->setText(2, dateTime.toString("yyyy-MM-dd hh:mm"));

			addTopLevelItem(item);
		}

		expandAll();
	}
	catch (const std::exception& e)
	{
		QMessageBox::critical(nullptr, tr("Error"),
			tr("Failed to read save directory: %1").arg(e.what()));
	}
}

void MemoryCardBrowser::navigateTo(const QString& path)
{
	if (!m_card)
		return;

	m_currentPath = path;

	if (path == "/")
	{
		loadRootDirectory();
	}
	else
	{
		loadSaveDirectory(path);
	}

	emit pathChanged(m_currentPath);
}

void MemoryCardBrowser::navigateUp()
{
	if (m_currentPath != "/")
	{
		navigateTo("/");
	}
}

bool MemoryCardBrowser::isInsideSave() const
{
	return m_currentPath != "/";
}

QString MemoryCardBrowser::getCurrentSavePath() const
{
	QTreeWidgetItem* item = currentItem();
	if (!item)
	{
		return QString();
	}

	if (m_currentPath == "/")
	{
		QString name = item->data(0, Qt::UserRole).toString();
		return "/" + name;
	}

	return m_currentPath;
}

QString MemoryCardBrowser::getSelectedPath() const
{
	QTreeWidgetItem* item = currentItem();
	if (!item)
	{
		return QString();
	}

	QString name = item->data(0, Qt::UserRole).toString();
	if (name == "..")
	{
		return QString();
	}

	if (m_currentPath == "/")
	{
		return "/" + name;
	}
	else
	{
		return m_currentPath + "/" + name;
	}
}

bool MemoryCardBrowser::hasSaveSelected() const
{
	QTreeWidgetItem* item = currentItem();
	if (!item)
		return false;

	QString name = item->data(0, Qt::UserRole).toString();
	return name != "..";
}

void MemoryCardBrowser::selectAll()
{
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		QString name = item->data(0, Qt::UserRole).toString();
		if (name != "..")
		{
			item->setSelected(true);
		}
	}
}

void MemoryCardBrowser::contextMenuEvent(QContextMenuEvent* event)
{
	QTreeWidgetItem* item = itemAt(event->pos());

	QMenu menu(this);

	QAction* importAction = menu.addAction(tr("Import File..."));
	connect(importAction, &QAction::triggered, this, [this]() {
		emit importArbitraryFileRequested(m_currentPath);
	});

	QAction* newFolderAction = menu.addAction(tr("New Folder..."));
	connect(newFolderAction, &QAction::triggered, this, [this]() {
		emit createFolderRequested(m_currentPath);
	});

	menu.addSeparator();

	if (m_currentPath == "/")
	{
		if (item)
		{
			QString name = item->data(0, Qt::UserRole).toString();
			bool isDir = item->data(0, Qt::UserRole + 1).toBool();

			if (name != "..")
			{
				if (isDir)
				{
					QAction* browseAction = menu.addAction(tr("Browse Contents"));
					connect(browseAction, &QAction::triggered, this, [this, name]() {
						navigateTo("/" + name);
					});

					QAction* exportAction = menu.addAction(tr("Export Save..."));
					connect(exportAction, &QAction::triggered, this, [this, name]() {
						emit exportSaveRequested("/" + name);
					});
				}
				else
				{
					QAction* exportAction = menu.addAction(tr("Export File..."));
					connect(exportAction, &QAction::triggered, this, [this, name]() {
						emit exportFileRequested(m_currentPath, name);
					});
				}

				menu.addSeparator();

				QAction* deleteAction = menu.addAction(tr("Delete"));
				connect(deleteAction, &QAction::triggered, this, [this, name]() {
					emit deleteSaveRequested("/" + name);
				});
			}
		}
	}
	else
	{
		if (item)
		{
			QString name = item->data(0, Qt::UserRole).toString();
			bool isDir = item->data(0, Qt::UserRole + 1).toBool();

			if (name == "..")
			{
				QAction* backAction = menu.addAction(tr("Go Back"));
				connect(backAction, &QAction::triggered, this, &MemoryCardBrowser::navigateUp);
			}
			else
			{
				if (!isDir)
				{
					QAction* exportAction = menu.addAction(tr("Export File..."));
					connect(exportAction, &QAction::triggered, this, [this, name]() {
						emit exportFileRequested(m_currentPath, name);
					});
				}
				
				QAction* deleteAction = menu.addAction(tr("Delete"));
				connect(deleteAction, &QAction::triggered, this, [this, name]() {
					QString fullPath = m_currentPath + "/" + name;
					if (m_currentPath == "/") fullPath = "/" + name;
					emit deleteSaveRequested(fullPath); 
				});
			}
		}

		menu.addSeparator();

		QAction* backAction = menu.addAction(tr("Go Back to Saves"));
		connect(backAction, &QAction::triggered, this, &MemoryCardBrowser::navigateUp);
	}

	if (!menu.isEmpty())
	{
		menu.exec(event->globalPos());
	}
}

void MemoryCardBrowser::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasUrls())
	{
		event->acceptProposedAction();
	}
}

void MemoryCardBrowser::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData->hasUrls())
	{
		QList<QUrl> urls = mimeData->urls();
		for (const QUrl& url : urls)
		{
			QString file = url.toLocalFile();

			if (m_currentPath == "/")
			{
				bool isSaveFile = file.endsWith(".psu", Qt::CaseInsensitive) ||
					file.endsWith(".max", Qt::CaseInsensitive) ||
					file.endsWith(".sps", Qt::CaseInsensitive) ||
					file.endsWith(".cbs", Qt::CaseInsensitive) ||
					file.endsWith(".xps", Qt::CaseInsensitive) ||
					file.endsWith(".psv", Qt::CaseInsensitive);

				if (isSaveFile)
				{
					emit saveFileDropped(file);
				}
				else
				{
					emit importFileRequested(m_currentPath, file);
				}
			}
			else
			{
				emit importFileRequested(m_currentPath, file);
			}
		}
	}
}
