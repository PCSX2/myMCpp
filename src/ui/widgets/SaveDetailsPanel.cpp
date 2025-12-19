// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "SaveDetailsPanel.h"
#include "IconWidget.h"
#include "Config.h"
#include "ps2mc.h"
#include "ps2iconsys.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QWidget>

SaveDetailsPanel::SaveDetailsPanel(Config* config, QWidget* parent)
	: QWidget(parent)
	, m_config(config)
	, iconLayout(nullptr)
	, iconWidget(nullptr)
	, iconContainer(nullptr)
{
	setupUI();
}

void SaveDetailsPanel::setupUI()
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	QGroupBox* iconGroup = new QGroupBox("Icon");
	iconLayout = new QVBoxLayout(iconGroup);

	createIconWidget();

	mainLayout->addWidget(iconGroup);

	QGroupBox* infoGroup = new QGroupBox("Details");
	QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);

	titleLabel = new QLabel("No save selected");
	titleLabel->setWordWrap(true);
	titleLabel->setMinimumHeight(40);
	titleLabel->setStyleSheet("font-weight: bold; font-size: 12pt;");
	infoLayout->addWidget(titleLabel);

	QFrame* separator = new QFrame();
	separator->setFrameShape(QFrame::HLine);
	separator->setFrameShadow(QFrame::Sunken);
	infoLayout->addWidget(separator);
	detailsLabel = new QLabel("No details available");
	detailsLabel->setWordWrap(true);
	detailsLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	infoLayout->addWidget(detailsLabel);
	mainLayout->addWidget(infoGroup);

	mainLayout->addStretch();
}

void SaveDetailsPanel::setSave(PS2MemoryCard* card, const QString& savePath,
	const QString& size, const QString& modified)
{
	if (!card)
	{
		clear();
		return;
	}

	currentCard = card;
	currentSavePath = savePath;
	currentSize = size;
	currentModified = modified;

	QString saveName = savePath;
	if (saveName.startsWith("/"))
	{
		saveName = saveName.mid(1);
	}

	QString fullTitle = saveName;
	try
	{
		std::string title = card->getSaveTitle(savePath.toStdString());
		std::string subtitle = card->getSaveSubtitle(savePath.toStdString());

		if (!title.empty() || !subtitle.empty())
		{
			fullTitle = QString::fromStdString(title);
			if (!subtitle.empty())
			{
				fullTitle += " " + QString::fromStdString(subtitle);
			}
		}
	}
	catch (...)
	{
	}

	titleLabel->setText(fullTitle);

	QString details = QString("Size: %1\nModified: %2").arg(size, modified);

	try
	{
		auto entries = card->listDir(savePath.toStdString());
		int fileCount = 0;

		for (const auto& entry : entries)
		{
			if (!(entry.mode & DF_DIR) && !(entry.mode & DF_HIDDEN))
			{
				fileCount++;
			}
		}

		if (fileCount > 0)
		{
			details += QString("\nFiles: %1").arg(fileCount);
		}
	}
	catch (...)
	{
	}

	detailsLabel->setText(details);

	try
	{
		auto iconData = card->getIconData(savePath.toStdString());

		if (iconData.empty())
		{
			qDebug() << "SaveDetailsPanel: No icon data found for" << savePath;
		}
		else
		{
			qDebug() << "SaveDetailsPanel: Icon data found, size:" << iconData.size();
		}

		if (!iconData.empty() && iconWidget)
		{
			if (iconWidget->loadIcon(iconData))
			{
				qDebug() << "SaveDetailsPanel: Icon loaded successfully into widget";
				PS2IconSys* iconSys = card->getIconSys(savePath.toStdString());
				if (iconSys)
				{
					iconWidget->applyConfigToRenderer(iconSys);
					iconWidget->setBackgroundFromIconSys(iconSys);
					qDebug() << "SaveDetailsPanel: Lighting and background configured from icon.sys";
					delete iconSys; // Clean up the pointer after use
				}
				else
				{
					qDebug() << "SaveDetailsPanel: No icon.sys found, using defaults";
					iconWidget->applyConfigToRenderer(nullptr);
					iconWidget->setBackgroundFromIconSys(nullptr);
				}

				iconWidget->setRotation(0.0f, 0.0f, 0.0f);
				iconWidget->setZoom(1.0f);
				return;
			}
			else
			{
				qDebug() << "SaveDetailsPanel: IconWidget failed to load icon";
			}
		}
	}
	catch (const std::exception& e)
	{
		qDebug() << "SaveDetailsPanel: Exception loading icon:" << e.what();
	}
	catch (...)
	{
		qDebug() << "SaveDetailsPanel: Unknown exception loading icon";
	}
}

void SaveDetailsPanel::clear()
{
	if (titleLabel)
		titleLabel->setText("No save selected");
	if (detailsLabel)
		detailsLabel->setText("No details available");
	currentCard = nullptr;
	currentSavePath.clear();
}

void SaveDetailsPanel::createIconWidget()
{
	if (iconContainer)
	{
		iconLayout->removeWidget(iconContainer);
		iconContainer->deleteLater();
		iconContainer = nullptr;
		iconWidget = nullptr;
	}

	iconWidget = new IconWidget(m_config, this);
	iconWidget->setMinimumSize(256, 256);
	iconWidget->setMaximumSize(256, 256);
	iconContainer = iconWidget;
	iconLayout->addWidget(iconContainer);
}

void SaveDetailsPanel::refreshConfig()
{
	createIconWidget();

	if (currentCard && !currentSavePath.isEmpty())
	{
		setSave(currentCard, currentSavePath, currentSize, currentModified);
	}
}
