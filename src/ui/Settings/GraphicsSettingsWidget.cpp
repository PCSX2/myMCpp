// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "GraphicsSettingsWidget.h"
#include "SettingsWindow.h"
#include "ui_GraphicsSettingsWidget.h"
#include "Config.h"
#include "TranslationManager.h"
#include "../../core/renderer/Renderer.h"

GraphicsSettingsWidget::GraphicsSettingsWidget(SettingsWindow* dialog, QWidget* parent)
	: SettingsWidget(dialog, parent)
	, ui(new Ui::GraphicsSettingsWidget)
{
	QWidget* container = new QWidget(this);
	ui->setupUi(container);

	registerHelp(ui->rendererCombo, tr("Renderer"), tr("Select the graphics API used for rendering the 3D icons."));
	registerHelp(ui->cameraCombo, tr("Camera Angle"), tr("Change the camera angle used to view the 3D icons."));
	registerHelp(ui->lightingCombo, tr("Lighting Mode"), tr("Select how the icons are lit."));
	registerHelp(ui->thumbnailSizeSpinner, tr("Thumbnail Size"), tr("Adjust the size of the save icons in the main view."));
	registerHelp(ui->antialiasingCombo, tr("Antialiasing"), tr("Enable multisample antialiasing for smoother edges. Higher values may impact performance."));
	registerHelp(ui->animateIconsCheck, tr("Animate Icons"), tr("Enable rotating animations for the 3D icons."));
	registerHelp(ui->fpsLimitSpinner, tr("FPS Limit"), tr("Set frame rate limit for icon preview. Set to 0 for unlimited FPS."));
	registerHelp(ui->vsyncCheck, tr("VSync"), tr("Synchronize frame rate with monitor refresh rate to prevent screen tearing."));

	ui->rendererCombo->clear();
#if defined(ENABLE_VULKAN) && !defined(__APPLE__)
	ui->rendererCombo->addItem("Vulkan", "vulkan");
#endif
	ui->rendererCombo->addItem("OpenGL", "opengl");
#if defined(__APPLE__)
	ui->rendererCombo->addItem("Metal", "metal");
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

	ui->antialiasingCombo->clear();
	ui->antialiasingCombo->addItem(tr("Off"), 0);
	ui->antialiasingCombo->addItem(tr("2x MSAA"), 2);
	ui->antialiasingCombo->addItem(tr("4x MSAA"), 4);
	ui->antialiasingCombo->addItem(tr("8x MSAA"), 8);

	connect(ui->rendererCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GraphicsSettingsWidget::onRendererChanged);
	connect(ui->cameraCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->lightingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->thumbnailSizeSpinner, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsWidget::settingChanged);
	connect(ui->antialiasingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->animateIconsCheck, &QCheckBox::toggled, this, &SettingsWidget::settingChanged);
	connect(ui->fpsLimitSpinner, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsWidget::settingChanged);
	connect(ui->vsyncCheck, &QCheckBox::toggled, this, &GraphicsSettingsWidget::onVSyncChanged);

	addTab(tr("Graphics"), container);
	loadSettings();
}

GraphicsSettingsWidget::~GraphicsSettingsWidget() = default;

void GraphicsSettingsWidget::onRendererChanged(int index)
{
	if (index < 0)
		return;

	emit settingChanged();
}

void GraphicsSettingsWidget::loadSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	QString r = QString::fromStdString(config->getRenderer()).toLower();
	int rIdxData = ui->rendererCombo->findData(r);
	if (rIdxData >= 0)
		ui->rendererCombo->setCurrentIndex(rIdxData);

	QString c = QString::fromStdString(config->getCameraMode()).toLower();
	int cIdx = ui->cameraCombo->findData(c);
	if (cIdx >= 0)
		ui->cameraCombo->setCurrentIndex(cIdx);

	QString l = QString::fromStdString(config->getLightingMode()).toLower();
	int lIdx = ui->lightingCombo->findData(l);
	if (lIdx >= 0)
		ui->lightingCombo->setCurrentIndex(lIdx);

	ui->thumbnailSizeSpinner->setValue(config->getThumbnailSize());

	int aaIdx = ui->antialiasingCombo->findData(config->getAntialiasing());
	if (aaIdx >= 0)
		ui->antialiasingCombo->setCurrentIndex(aaIdx);

	ui->animateIconsCheck->setChecked(config->getAnimateIcons());
	ui->fpsLimitSpinner->setValue(config->getMaxFPS() <= 0 ? 0 : config->getMaxFPS());
	ui->vsyncCheck->setChecked(config->getVSync());
}

void GraphicsSettingsWidget::saveSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	config->setRenderer(ui->rendererCombo->currentData().toString().toStdString());
	config->setCameraMode(ui->cameraCombo->currentData().toString().toStdString());
	config->setLightingMode(ui->lightingCombo->currentData().toString().toStdString());
	config->setThumbnailSize(ui->thumbnailSizeSpinner->value());
	config->setAntialiasing(ui->antialiasingCombo->currentData().toInt());
	config->setAnimateIcons(ui->animateIconsCheck->isChecked());
	config->setMaxFPS(ui->fpsLimitSpinner->value());
	config->setVSync(ui->vsyncCheck->isChecked());
}

void GraphicsSettingsWidget::restoreDefaults()
{
	ui->rendererCombo->setCurrentIndex(0);
	ui->cameraCombo->setCurrentIndex(0);
	ui->lightingCombo->setCurrentIndex(0);
	ui->thumbnailSizeSpinner->setValue(64);
	ui->antialiasingCombo->setCurrentIndex(0);
	ui->animateIconsCheck->setChecked(true);
	ui->fpsLimitSpinner->setValue(30);
	ui->vsyncCheck->setChecked(true);
}

void GraphicsSettingsWidget::onVSyncChanged(bool enabled)
{
	// Apply VSync setting to all active renderers
	RendererFactory::applyVSyncToAll(enabled);

	// Also emit generic setting changed for config saving
	emit settingChanged();
}
