// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "SaveDetailsPanel.h"
#include "IconWidget.h"
#include "Config.h"
#include "ps2mc.h"
#include "ps2iconsys.h"
#include "TranslationManager.h"

SaveDetailsPanel::SaveDetailsPanel(QWidget* parent)
	: QWidget(parent)
	, m_config(nullptr)
	, iconWidget(nullptr)
	, ui(new Ui::SaveDetailsPanel)
{
	ui->setupUi(this);

	if (ui->iconContainer)
	{
		ui->iconLayout->removeWidget(ui->iconContainer);
		delete ui->iconContainer;
		ui->iconContainer = nullptr;
	}

	connect(&TranslationManager::instance(), &TranslationManager::languageChanged, this, [this]() {
		ui->retranslateUi(this);
	});

	this->hide();
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

	this->show();

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
	ui->dirNameLabel->setText(saveName);

	QString details = tr("Size: %1\nModified: %2").arg(size, modified);

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
			details += tr("\nFiles: %1").arg(fileCount);
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
			if (iconWidget)
				iconWidget->hide();
		}
		else
		{
		}

		if (!iconData.empty() && iconWidget)
		{
			if (iconWidget->loadIcon(iconData))
			{
				iconWidget->show();
				PS2IconSys* iconSys = card->getIconSys(savePath.toStdString());
				if (iconSys)
				{
					iconWidget->applyConfigToRenderer(iconSys);
					iconWidget->setBackgroundFromIconSys(iconSys);
					delete iconSys; // Clean up the pointer after use
				}
				else
				{
					iconWidget->applyConfigToRenderer(nullptr);
					iconWidget->setBackgroundFromIconSys(nullptr);
				}

				iconWidget->setRotation(0.0f, 0.0f, 0.0f);
				iconWidget->setZoom(1.0f);
				return;
			}
			else
			{
				iconWidget->hide();
			}
		}
	}
	catch (const std::exception& e)
	{
		if (iconWidget)
			iconWidget->hide();
	}
	catch (...)
	{
		if (iconWidget)
			iconWidget->hide();
	}
}

void SaveDetailsPanel::clear()
{
	if (ui->titleLabel)
		ui->titleLabel->setText(tr("No save selected"));
	if (ui->dirNameLabel)
		ui->dirNameLabel->setText("");
	if (ui->detailsLabel)
		ui->detailsLabel->setText(tr("No details available"));
	if (iconWidget)
		iconWidget->hide();
	currentCard = nullptr;
	currentSavePath.clear();
	this->hide();
}

void SaveDetailsPanel::createIconWidget()
{
	if (iconWidget)
	{
		ui->iconLayout->removeWidget(iconWidget);
		delete iconWidget;
		iconWidget = nullptr;
	}

	iconWidget = new IconWidget(m_config, this);
	if (m_config)
		m_lastRendererType = m_config->getRenderer();
	else
		m_lastRendererType = "vulkan";

	iconWidget->setMinimumSize(256, 256);
	iconWidget->setMaximumSize(256, 256);
	ui->iconLayout->addWidget(iconWidget);
	iconWidget->hide();
}

void SaveDetailsPanel::refreshConfig()
{
	std::string currentRenderer = m_config ? m_config->getRenderer() : "vulkan";
	if (!iconWidget || currentRenderer != m_lastRendererType)
	{
		createIconWidget();
	}

	if (!currentCard || currentSavePath.isEmpty())
	{
		if (iconWidget)
			iconWidget->hide();
	}

	if (currentCard && !currentSavePath.isEmpty())
	{
		setSave(currentCard, currentSavePath, currentSize, currentModified);
	}
}
