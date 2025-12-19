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

#include "SaveDetailsPanel.h"
#include "IconWidget.h"
#include "Config.h"
#include "ps2mc.h"
#include "ps2iconsys.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QWidget>

SaveDetailsPanel::SaveDetailsPanel(QWidget* parent)
	: QWidget(parent)
	, m_config(nullptr)
	, iconWidget(nullptr)
	, ui(new Ui::SaveDetailsPanel)
{
	ui->setupUi(this);

	// The UI file creates a placeholder iconContainer. We will use the layout to place our IconWidget.
	// Ideally we would promote the widget in Designer but IconWidget takes custom constructor args (config).
	// So we will replace the placeholder or add to layout.

	// Let's remove the placeholder
	if (ui->iconContainer)
	{
		ui->iconLayout->removeWidget(ui->iconContainer);
		delete ui->iconContainer;
		ui->iconContainer = nullptr;
	}
}

void SaveDetailsPanel::setConfig(Config* config)
{
	m_config = config;
	createIconWidget();
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

	ui->titleLabel->setText(fullTitle);

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

	ui->detailsLabel->setText(details);

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
	if (ui->titleLabel)
		ui->titleLabel->setText("No save selected");
	if (ui->detailsLabel)
		ui->detailsLabel->setText("No details available");
	currentCard = nullptr;
	currentSavePath.clear();
}

void SaveDetailsPanel::createIconWidget()
{
	if (iconWidget)
	{
		ui->iconLayout->removeWidget(iconWidget);
		delete iconWidget;
		iconWidget = nullptr;
	}

	// Only create if we have config, or create with nullptr if safely handled
	iconWidget = new IconWidget(m_config, this);
	if (m_config)
		m_lastRendererType = m_config->getRenderer();
	else
		m_lastRendererType = "vulkan";

	iconWidget->setMinimumSize(256, 256);
	iconWidget->setMaximumSize(256, 256);
	ui->iconLayout->addWidget(iconWidget);
}

void SaveDetailsPanel::refreshConfig()
{
	std::string currentRenderer = m_config ? m_config->getRenderer() : "vulkan";
	if (!iconWidget || currentRenderer != m_lastRendererType)
	{
		createIconWidget();
	}

	if (currentCard && !currentSavePath.isEmpty())
	{
		setSave(currentCard, currentSavePath, currentSize, currentModified);
	}
}
