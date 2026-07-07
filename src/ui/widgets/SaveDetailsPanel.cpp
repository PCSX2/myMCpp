// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "SaveDetailsPanel.h"
#include "IconWidget.h"
#include "Config.h"
#include <QStyle>
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

	connect(ui->playPauseButton, &QPushButton::clicked, this, [this]() {
		if (!iconWidget)
			return;
		iconWidget->setAnimationEnabled(!iconWidget->isAnimationEnabled());
		updatePlayPauseButton();
	});

	connect(ui->resetViewButton, &QPushButton::clicked, this, [this]() {
		if (!iconWidget)
			return;
		iconWidget->resetCamera();
	});

	connect(ui->zoomInButton, &QPushButton::clicked, this, [this]() {
		if (!iconWidget)
			return;
		iconWidget->zoomIn();
	});

	connect(ui->zoomOutButton, &QPushButton::clicked, this, [this]() {
		if (!iconWidget)
			return;
		iconWidget->zoomOut();
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
				updatePlayPauseButton();
				return;
			}
			else
			{
				iconWidget->hide();
			}
		}
	}
	catch (const std::exception&)
	{
		if (iconWidget)
			iconWidget->hide();
	}
	catch (...)
	{
		if (iconWidget)
			iconWidget->hide();
	}
	updatePlayPauseButton();
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
	updatePlayPauseButton();
	currentCard = nullptr;
	currentSavePath.clear();
	this->hide();
}

void SaveDetailsPanel::updatePlayPauseButton()
{
	const bool visible = iconWidget && iconWidget->isVisible();
	ui->playPauseButton->setVisible(visible);
	ui->resetViewButton->setVisible(visible);
	ui->zoomInButton->setVisible(visible);
	ui->zoomOutButton->setVisible(visible);
	if (!visible)
		return;

	const bool animating = iconWidget->isAnimationEnabled();
	ui->playPauseButton->setIcon(
		style()->standardIcon(animating ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
	ui->playPauseButton->setToolTip(
		animating ? tr("Pause animation") : tr("Play animation"));
	ui->resetViewButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
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
	{
		m_lastRendererType = m_config->getRenderer();
		m_lastAdapter = m_config->getAdapter();
	}
	else
	{
		m_lastRendererType = "vulkan";
		m_lastAdapter = "";
	}

	iconWidget->setMinimumSize(128, 128);
	QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
	sp.setHeightForWidth(true);
	iconWidget->setSizePolicy(sp);
	ui->iconLayout->insertWidget(0, iconWidget);
	iconWidget->hide();

	emit iconWidgetChanged(iconWidget);
}

void SaveDetailsPanel::refreshConfig()
{
	std::string currentRenderer = m_config ? m_config->getRenderer() : "vulkan";
	std::string currentAdapter = m_config ? m_config->getAdapter() : "";
	if (!iconWidget || currentRenderer != m_lastRendererType || currentAdapter != m_lastAdapter)
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
