// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "SettingsDialog.h"
#include "Config.h"
#include "Logger.h"
#include "TranslationManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QApplication>
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QGroupBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>

SettingsDialog::SettingsDialog(Config* config, QWidget* parent)
	: QDialog(parent)
	, m_config(config)
	, ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);
	loadCurrentSettings();

	connect(ui->categoryList, &QListWidget::itemClicked, this, &SettingsDialog::onCategorySelected);
	connect(ui->rendererCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
		this, &SettingsDialog::onRendererChanged);
	connect(ui->browseButton, &QPushButton::clicked, this, &SettingsDialog::onBrowseMemoryCardPath);
	connect(this, &QDialog::accepted, this, &SettingsDialog::onAccepted);
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::onCategorySelected(QListWidgetItem* item)
{
	int index = ui->categoryList->row(item);
	ui->settingsStack->setCurrentIndex(index);
}

void SettingsDialog::loadCurrentSettings()
{
	if (!m_config)
		return;

	ui->darkModeCheck->setChecked(m_config->getDarkMode());
	ui->darkModeCheck->setChecked(m_config->getDarkMode());
	ui->thumbnailSizeSpinner->setValue(m_config->getThumbnailSize());

	// Load languages
	ui->languageCombo->clear();
	ui->languageCombo->addItem("English", "en");
	// We will add more languages as translations become available

	const QString currentLang = QString::fromStdString(m_config->getLanguage());
	int langIndex = ui->languageCombo->findData(currentLang);
	if (langIndex >= 0)
		ui->languageCombo->setCurrentIndex(langIndex);

	std::string renderer_str = m_config->getRenderer();
	QString rendererKey = QString::fromStdString(renderer_str).toLower();

	int rendererIndex = ui->rendererCombo->findText(rendererKey, Qt::MatchFixedString); // Omitting Qt::MatchCaseSensitive implies case insensitivity

	ui->rendererCombo->clear();
#if !defined(__APPLE__)
	ui->rendererCombo->addItem("Vulkan", "vulkan");
#endif
	ui->rendererCombo->addItem("OpenGL", "opengl");
#if defined(__APPLE__)
	ui->rendererCombo->addItem("Metal", "metal");
#endif

	ui->lightingCombo->setItemData(0, "icon");
	ui->lightingCombo->setItemData(1, "off");
	ui->lightingCombo->setItemData(2, "alt1");
	ui->lightingCombo->setItemData(3, "alt2");

	ui->cameraCombo->setItemData(0, "default");
	ui->cameraCombo->setItemData(1, "flat");
	ui->cameraCombo->setItemData(2, "near");
	ui->cameraCombo->setItemData(3, "high");

	rendererIndex = ui->rendererCombo->findData(rendererKey);
	if (rendererIndex >= 0)
		ui->rendererCombo->setCurrentIndex(rendererIndex);

	ui->animateIconsCheck->setChecked(m_config->getAnimateIcons());

	const QString lighting = QString::fromStdString(m_config->getLightingMode()).toLower();
	int lightingIndex = ui->lightingCombo->findData(lighting);
	if (lightingIndex >= 0)
		ui->lightingCombo->setCurrentIndex(lightingIndex);

	const QString camera = QString::fromStdString(m_config->getCameraMode()).toLower();
	int cameraIndex = ui->cameraCombo->findData(camera);
	if (cameraIndex >= 0)
		ui->cameraCombo->setCurrentIndex(cameraIndex);

	ui->warnOnDeleteCheck->setChecked(m_config->getWarnOnDelete());
	ui->confirmShutdownCheck->setChecked(m_config->getConfirmShutdown());
	ui->hideToTrayCheck->setChecked(m_config->getHideToTrayOnClose());
	ui->memoryCardPathEdit->setText(QString::fromStdString(m_config->getMemoryCardFolder()));
	ui->enableDebugLogCheck->setChecked(m_config->getDebugLogging());
}

void SettingsDialog::onAccepted()
{
	if (!m_config)
	{
		Logger::error("[SettingsDialog] No config object!");
		return;
	}

	m_config->setDarkMode(ui->darkModeCheck->isChecked());
	m_config->setDarkMode(ui->darkModeCheck->isChecked());
	m_config->setThumbnailSize(ui->thumbnailSizeSpinner->value());
	m_config->setLanguage(ui->languageCombo->currentData().toString().toStdString());

	QString renderer = ui->rendererCombo->currentData().toString();
	m_config->setRenderer(renderer.toLower().toStdString());
	m_config->setAnimateIcons(ui->animateIconsCheck->isChecked());
	m_config->setLightingMode(ui->lightingCombo->currentData().toString().toStdString());
	m_config->setCameraMode(ui->cameraCombo->currentData().toString().toStdString());

	m_config->setWarnOnDelete(ui->warnOnDeleteCheck->isChecked());
	m_config->setConfirmShutdown(ui->confirmShutdownCheck->isChecked());
	m_config->setHideToTrayOnClose(ui->hideToTrayCheck->isChecked());
	m_config->setMemoryCardFolder(ui->memoryCardPathEdit->text().toStdString());
	m_config->setDebugLogging(ui->enableDebugLogCheck->isChecked());

	m_config->save();

	TranslationManager::instance().loadLanguage(m_config->getLanguage());
}

void SettingsDialog::onBrowseMemoryCardPath()
{
	QString dir = QFileDialog::getExistingDirectory(this,
		tr("Select Memory Card Folder"), ui->memoryCardPathEdit->text());
	if (!dir.isEmpty())
	{
		ui->memoryCardPathEdit->setText(dir);
	}
}

void SettingsDialog::onRendererChanged(const QString& renderer)
{
	Logger::info("Renderer selected: {}", renderer.toStdString());
}
