// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "SaveDetailsPanel.h"
#include "IconWidget.h"
#include "TranslationManager.h"

#include "core/formats/PS2MemoryCard.h"
#include "core/formats/PS2Icon.h"
#include "core/formats/PS2IconSys.h"

#include "common/Config.h"

SaveDetailsPanel::SaveDetailsPanel(QWidget* parent)
	: QWidget(parent)
	, m_config(nullptr)
	, iconWidget(nullptr)
	, ui(new Ui::SaveDetailsPanel)
{
	ui->setupUi(this);

	connect(&TranslationManager::instance(), &TranslationManager::languageChanged, this, [this]() {
		ui->retranslateUi(this);
		updateIconControls();
	});

	connect(ui->iconTypeComboBox, &QComboBox::currentIndexChanged, this, [this](int) {
		loadSelectedIcon(true);
	});

	connect(ui->playPauseButton, &QPushButton::clicked, this, [this]() {
		if (!iconWidget)
			return;
		iconWidget->setAnimationEnabled(!iconWidget->isAnimationEnabled());
		updateIconControls();
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

	ui->zoomInButton->setIcon(QIcon::fromTheme(QStringLiteral("add-line")));
	ui->zoomInButton->setText(QString());
	ui->zoomOutButton->setIcon(QIcon::fromTheme(QStringLiteral("subtract-line")));
	ui->zoomOutButton->setText(QString());
	ui->resetViewButton->setIcon(QIcon::fromTheme(QStringLiteral("reset-right-line")));

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

	const bool sameSave = currentCard == card && currentSavePath == savePath;
	const int previousType = sameSave ? ui->iconTypeComboBox->currentData().toInt() : static_cast<int>(PS2MemoryCard::IconType::Idle);
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

	ui->titleLabel->setText(fullTitle);
	ui->dirNameLabel->setText(saveName);

	QString details = tr("Size: %1\nModified: %2").arg(size, modified);

	{
		auto entries = card->listDir(savePath.toStdString());
		int fileCount = 0;

		if (!card->GetError().IsValid())
		{
			for (const auto& entry : entries)
			{
				if (!(entry.mode & DF_DIR) && !(entry.mode & DF_HIDDEN))
				{
					fileCount++;
				}
			}
		}

		if (fileCount > 0)
		{
			details += tr("\nFiles: %1").arg(fileCount);
		}
	}

	ui->detailsLabel->setText(details);

	{
		const QSignalBlocker blocker(ui->iconTypeComboBox);
		ui->iconTypeComboBox->clear();
		for (int type = 0; type < static_cast<int>(m_iconData.size()); ++type)
		{
			m_iconData[type] = card->getIconData(savePath.toStdString(),
				static_cast<PS2MemoryCard::IconType>(type));
			if (m_iconData[type].empty() ||
				std::find(m_iconData.begin(), m_iconData.begin() + type, m_iconData[type]) != m_iconData.begin() + type)
			{
				m_iconData[type].clear();
				continue;
			}
			PS2Icon::Icon icon;
			if (icon.load(m_iconData[type]))
				ui->iconTypeComboBox->addItem(QString(), type);
			else
				m_iconData[type].clear();
		}
		const int previousIndex = ui->iconTypeComboBox->findData(previousType);
		if (previousIndex >= 0)
			ui->iconTypeComboBox->setCurrentIndex(previousIndex);
	}
	loadSelectedIcon();
}

void SaveDetailsPanel::loadSelectedIcon(bool preserveState)
{
	if (!iconWidget)
	{
		updateIconControls();
		return;
	}

	const bool animating = iconWidget->isAnimationEnabled();
	if (currentCard && ui->iconTypeComboBox->currentIndex() >= 0 &&
		iconWidget->loadIcon(m_iconData[ui->iconTypeComboBox->currentData().toInt()]))
	{
		std::unique_ptr<PS2IconSys> iconSys(currentCard->getIconSys(currentSavePath.toStdString()));
		iconWidget->applyConfigToRenderer(iconSys.get());
		iconWidget->setBackgroundFromIconSys(iconSys.get());
		if (preserveState)
			iconWidget->setAnimationEnabled(animating);
		else
		{
			iconWidget->setRotation(0.0f, 0.0f, 0.0f);
			iconWidget->setZoom(1.0f);
		}
		iconWidget->show();
	}
	else
		iconWidget->hide();
	updateIconControls();
}

void SaveDetailsPanel::clear()
{
	{
		const QSignalBlocker blocker(ui->iconTypeComboBox);
		ui->iconTypeComboBox->clear();
	}
	for (auto& iconData : m_iconData)
		iconData.clear();
	if (ui->titleLabel)
		ui->titleLabel->setText(tr("No save selected"));
	if (ui->dirNameLabel)
		ui->dirNameLabel->setText("");
	if (ui->detailsLabel)
		ui->detailsLabel->setText(tr("No details available"));
	if (iconWidget)
		iconWidget->hide();
	updateIconControls();
	currentCard = nullptr;
	currentSavePath.clear();
	this->hide();
}

void SaveDetailsPanel::updateIconControls()
{
	const std::array<QString, 3> labels = {tr("Idle"), tr("Copy"), tr("Delete")};
	for (int index = 0; index < ui->iconTypeComboBox->count(); ++index)
		ui->iconTypeComboBox->setItemText(index, labels[ui->iconTypeComboBox->itemData(index).toInt()]);
	ui->iconTypeComboBox->setVisible(ui->iconTypeComboBox->count() > 1);

	const bool visible = iconWidget && iconWidget->isVisible();
	ui->playPauseButton->setVisible(visible);
	ui->resetViewButton->setVisible(visible);
	ui->zoomInButton->setVisible(visible);
	ui->zoomOutButton->setVisible(visible);
	if (!visible)
		return;

	const bool animating = iconWidget->isAnimationEnabled();
	ui->playPauseButton->setIcon(
		QIcon::fromTheme(animating ? QStringLiteral("pause-line") : QStringLiteral("play-fill")));
	ui->playPauseButton->setToolTip(
		animating ? tr("Pause animation") : tr("Play animation"));
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
		m_lastRendererType = m_config->Graphics.Renderer;
		m_lastAdapter = m_config->Graphics.Adapter;
	}
	else
	{
		m_lastRendererType = RendererType::Automatic;
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
	RendererType currentRenderer = m_config ? m_config->Graphics.Renderer : RendererType::Automatic;
	std::string currentAdapter = m_config ? m_config->Graphics.Adapter : "";
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
