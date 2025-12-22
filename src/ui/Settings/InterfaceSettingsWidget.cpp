// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "InterfaceSettingsWidget.h"
#include "SettingsWindow.h"
#include "ui_InterfaceSettingsWidget.h"
#include "Config.h"
#include "TranslationManager.h"
#include "../Themes.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QFormLayout>

InterfaceSettingsWidget::InterfaceSettingsWidget(SettingsWindow* dialog, QWidget* parent)
	: SettingsWidget(dialog, parent)
	, ui(new Ui::InterfaceSettingsWidget)
{
	QWidget* container = new QWidget(this);
	ui->setupUi(container);

	registerHelp(ui->languageCombo, tr("Language"), tr("Select the language for the application interface."));
	registerHelp(ui->themeCombo, tr("Theme"), tr("Select the color theme for the application."));
	registerHelp(ui->thumbnailSizeSpinner, tr("Thumbnail Size"), tr("Adjust the size of the save icons in the main view."));
	registerHelp(ui->rendererCombo, tr("Renderer"), tr("Select the graphics API used for rendering the 3D icons."));
	registerHelp(ui->cameraCombo, tr("Camera Angle"), tr("Change the camera angle used to view the 3D icons."));
	registerHelp(ui->lightingCombo, tr("Lighting Mode"), tr("Select how the icons are lit."));
	registerHelp(ui->animateIconsCheck, tr("Animate Icons"), tr("Enable rotating animations for the 3D icons."));

	ui->languageCombo->clear();
	ui->languageCombo->addItem("English (US)", "en");

	ui->themeCombo->clear();
	ui->themeCombo->addItem(tr("None"), "none");
	ui->themeCombo->addItem(tr("Dark (Default)"), "dark");
	ui->themeCombo->addItem(tr("Light"), "light");
	ui->themeCombo->addItem(tr("Pizza Brown [Light]"), "pizzabrown");
	ui->themeCombo->addItem(tr("Grey Matter [Dark]"), "greymatter");
	ui->themeCombo->addItem(tr("Cobalt Sky [Dark]"), "cobaltsky");
	ui->themeCombo->addItem(tr("AMOLED [Black]"), "amoled");
	#ifdef _WIN32
	ui->themeCombo->addItem(tr("Windows Vista"), "windowsvista");
	#endif

	ui->rendererCombo->clear();
#if !defined(__APPLE__)
	ui->rendererCombo->addItem("Vulkan", "vulkan");
#endif
	ui->rendererCombo->addItem("OpenGL", "opengl");
#if defined(__APPLE__)
	ui->rendererCombo->addItem("Metal", "metal");
#endif
#if defined(_WIN32)
	ui->rendererCombo->addItem("DirectX 12", "dx12");
#endif

	ui->cameraCombo->clear();
	ui->cameraCombo->addItem(tr("Default"), "default");
	ui->cameraCombo->addItem(tr("Flat"), "flat");
	ui->cameraCombo->addItem(tr("Near"), "near");
	ui->cameraCombo->addItem(tr("High"), "high");

	ui->lightingCombo->clear();
	ui->lightingCombo->addItem(tr("Icon Lighting"), "icon");
	ui->lightingCombo->addItem(tr("Lighting Off"), "off");
	ui->lightingCombo->addItem(tr("Alternate 1"), "alt1");
	ui->lightingCombo->addItem(tr("Alternate 2"), "alt2");

	connect(ui->themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->thumbnailSizeSpinner, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsWidget::settingChanged);
	connect(ui->rendererCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->cameraCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->lightingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->animateIconsCheck, &QCheckBox::toggled, this, &SettingsWidget::settingChanged);

	addTab(tr("Interface"), container);
	loadSettings();
}

InterfaceSettingsWidget::~InterfaceSettingsWidget() = default;

void InterfaceSettingsWidget::loadSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config) return;

	int langIdx = ui->languageCombo->findData(QString::fromStdString(config->getLanguage()));
	if (langIdx >= 0) ui->languageCombo->setCurrentIndex(langIdx);

	QString t = QString::fromStdString(config->getTheme()).toLower();
	int tIdx = ui->themeCombo->findData(t);
	if (tIdx >= 0) ui->themeCombo->setCurrentIndex(tIdx);

	ui->thumbnailSizeSpinner->setValue(config->getThumbnailSize());

	QString r = QString::fromStdString(config->getRenderer()).toLower();
	int rIdxData = ui->rendererCombo->findData(r);
	if (rIdxData >= 0) ui->rendererCombo->setCurrentIndex(rIdxData);

	QString c = QString::fromStdString(config->getCameraMode()).toLower();
	int cIdx = ui->cameraCombo->findData(c);
	if (cIdx >= 0) ui->cameraCombo->setCurrentIndex(cIdx);
	QString l = QString::fromStdString(config->getLightingMode()).toLower();
	int lIdx = ui->lightingCombo->findData(l);
	if (lIdx >= 0) ui->lightingCombo->setCurrentIndex(lIdx);

	ui->animateIconsCheck->setChecked(config->getAnimateIcons());
}

void InterfaceSettingsWidget::saveSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config) return;

	config->setLanguage(ui->languageCombo->currentData().toString().toStdString());
	config->setTheme(ui->themeCombo->currentData().toString().toStdString());
	config->setThumbnailSize(ui->thumbnailSizeSpinner->value());

	config->setRenderer(ui->rendererCombo->currentData().toString().toStdString());
	config->setCameraMode(ui->cameraCombo->currentData().toString().toStdString());
	config->setLightingMode(ui->lightingCombo->currentData().toString().toStdString());
	config->setAnimateIcons(ui->animateIconsCheck->isChecked());
}

void InterfaceSettingsWidget::restoreDefaults()
{
	int enIndex = ui->languageCombo->findData("en");
	if (enIndex >= 0) ui->languageCombo->setCurrentIndex(enIndex);

	int darkIndex = ui->themeCombo->findData("dark");
	if (darkIndex >= 0) ui->themeCombo->setCurrentIndex(darkIndex);
	ui->thumbnailSizeSpinner->setValue(64);

	ui->cameraCombo->setCurrentIndex(0); // Default
	ui->lightingCombo->setCurrentIndex(0); // Icon
	ui->animateIconsCheck->setChecked(true);
}
