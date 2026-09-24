// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "GraphicsSettingsWidget.h"
#include "SettingsWindow.h"
#include "ui_GraphicsSettingsWidget.h"

#include "TranslationManager.h"

#include "core/renderer/Renderer.h"

#include "common/Config.h"

GraphicsSettingsWidget::GraphicsSettingsWidget(SettingsWindow* dialog, QWidget* parent)
	: SettingsWidget(dialog, parent)
	, ui(new Ui::GraphicsSettingsWidget)
{
	m_rootWidget = new QWidget(this);
	ui->setupUi(m_rootWidget);

	registerHelp(ui->rendererCombo, tr("Renderer"), tr("Select the graphics API used for rendering the 3D icons."));
	registerHelp(ui->adapterCombo, tr("Graphics Adapter"), tr("Select the graphics adapter (GPU) to use for rendering."));
	registerHelp(ui->cameraCombo, tr("Camera Angle"), tr("Change the camera angle used to view the 3D icons."));
	registerHelp(ui->lightingCombo, tr("Lighting Mode"), tr("Select how the icons are lit."));
	registerHelp(ui->animateIconsCheck, tr("Animate Icons"), tr("Enable rotating animations for the 3D icons."));
	registerHelp(ui->fpsLimitSpinner, tr("FPS Limit"), tr("Set frame rate limit for icon preview. Set to 0 for unlimited FPS."));
	registerHelp(ui->vsyncCheck, tr("VSync"), tr("Synchronize frame rate with monitor refresh rate to prevent screen tearing."));

	ui->rendererCombo->clear();
	ui->rendererCombo->addItem(tr("Automatic"), static_cast<int>(RendererType::Automatic));
#if defined(ENABLE_VULKAN)
	ui->rendererCombo->addItem(tr("Vulkan"), static_cast<int>(RendererType::Vulkan));
#endif
#if defined(ENABLE_OPENGL)
	ui->rendererCombo->addItem(tr("OpenGL"), static_cast<int>(RendererType::OpenGL));
#endif
#if defined(ENABLE_METAL)
	ui->rendererCombo->addItem(tr("Metal"), static_cast<int>(RendererType::Metal));
#endif

	ui->cameraCombo->clear();
	ui->cameraCombo->addItem(tr("Default"), static_cast<int>(CameraMode::Default));
	ui->cameraCombo->addItem(tr("Flat"), static_cast<int>(CameraMode::Flat));
	ui->cameraCombo->addItem(tr("Near"), static_cast<int>(CameraMode::Near));
	ui->cameraCombo->addItem(tr("High"), static_cast<int>(CameraMode::High));

	ui->lightingCombo->clear();
	ui->lightingCombo->addItem(tr("Icon Lighting"), static_cast<int>(LightingMode::Icon));
	ui->lightingCombo->addItem(tr("Lighting Off"), static_cast<int>(LightingMode::Off));
	ui->lightingCombo->addItem(tr("Alternate 1"), static_cast<int>(LightingMode::Alternate1));
	ui->lightingCombo->addItem(tr("Alternate 2"), static_cast<int>(LightingMode::Alternate2));

	connect(ui->rendererCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GraphicsSettingsWidget::onRendererChanged);
	connect(ui->adapterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->cameraCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->lightingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::settingChanged);
	connect(ui->animateIconsCheck, &QCheckBox::toggled, this, &SettingsWidget::settingChanged);
	connect(ui->fpsLimitSpinner, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsWidget::settingChanged);
	connect(ui->vsyncCheck, &QCheckBox::toggled, this, &GraphicsSettingsWidget::onVSyncChanged);

	addTab(tr("Graphics"), m_rootWidget);
	loadSettings();
}

GraphicsSettingsWidget::~GraphicsSettingsWidget() = default;

void GraphicsSettingsWidget::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		if (ui && m_rootWidget)
		{
			ui->retranslateUi(m_rootWidget);
		}
	}
	SettingsWidget::changeEvent(event);
}

void GraphicsSettingsWidget::onRendererChanged(int index)
{
	if (index < 0)
		return;

	updateAdapterComboState();
	emit settingChanged();
}

void GraphicsSettingsWidget::updateAdapterComboState()
{
	const RendererType renderer = static_cast<RendererType>(ui->rendererCombo->currentData().toInt());
	const QString selectedAdapter = ui->adapterCombo->currentData().toString();

	ui->adapterCombo->clear();
	ui->adapterCombo->addItem(tr("Default Adapter"), "");
	for (const std::string& name : RendererFactory::getAvailableAdapters(renderer))
	{
		const QString adapter = QString::fromStdString(name);
		ui->adapterCombo->addItem(adapter, adapter);
	}

	const int adapterIndex = ui->adapterCombo->findData(selectedAdapter);
	if (adapterIndex >= 0)
		ui->adapterCombo->setCurrentIndex(adapterIndex);

	const bool isGL = (renderer == RendererType::OpenGL);
	ui->adapterLabel->setVisible(!isGL);
	ui->adapterCombo->setVisible(!isGL);
}

void GraphicsSettingsWidget::loadSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	int rIdxData = ui->rendererCombo->findData(static_cast<int>(config->Graphics.Renderer));
	if (rIdxData >= 0)
		ui->rendererCombo->setCurrentIndex(rIdxData);

	updateAdapterComboState();

	QString adapter = QString::fromStdString(config->Graphics.Adapter);
	int adapterIdx = ui->adapterCombo->findData(adapter);
	if (adapterIdx >= 0)
		ui->adapterCombo->setCurrentIndex(adapterIdx);
	else
		ui->adapterCombo->setCurrentIndex(0);

	int cIdx = ui->cameraCombo->findData(static_cast<int>(config->Graphics.Camera));
	if (cIdx >= 0)
		ui->cameraCombo->setCurrentIndex(cIdx);

	int lIdx = ui->lightingCombo->findData(static_cast<int>(config->Graphics.Lighting));
	if (lIdx >= 0)
		ui->lightingCombo->setCurrentIndex(lIdx);

	ui->animateIconsCheck->setChecked(config->Graphics.AnimateIcons);
	ui->fpsLimitSpinner->setValue(config->Performance.MaxFPS <= 0 ? 0 : config->Performance.MaxFPS);
	ui->vsyncCheck->setChecked(config->Graphics.VSync);
}

void GraphicsSettingsWidget::saveSettings()
{
	Config* config = m_dialog->getConfig();
	if (!config)
		return;

	config->Graphics.Renderer = static_cast<RendererType>(ui->rendererCombo->currentData().toInt());
	config->Graphics.Adapter = ui->adapterCombo->currentData().toString().toStdString();
	config->Graphics.Camera = static_cast<CameraMode>(ui->cameraCombo->currentData().toInt());
	config->Graphics.Lighting = static_cast<LightingMode>(ui->lightingCombo->currentData().toInt());
	config->Graphics.AnimateIcons = ui->animateIconsCheck->isChecked();
	config->Performance.MaxFPS = ui->fpsLimitSpinner->value();
	config->Graphics.VSync = ui->vsyncCheck->isChecked();
}

void GraphicsSettingsWidget::onVSyncChanged(bool enabled)
{
	// Apply VSync setting to all active renderers
	RendererFactory::applyVSyncToAll(enabled);

	// Also emit generic setting changed for config saving
	emit settingChanged();
}
