// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "SettingsDialog.h"
#include "Config.h"
#include "../../common/Logger.h"
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
	, categoryList(nullptr)
	, settingsStack(nullptr)
	, darkModeCheck(nullptr)
	, thumbnailSizeSpinner(nullptr)
	, rendererCombo(nullptr)
	, animateIconsCheck(nullptr)
	, lightingCombo(nullptr)
	, cameraCombo(nullptr)
	, warnOnDeleteCheck(nullptr)
	, confirmShutdownCheck(nullptr)
	, hideToTrayCheck(nullptr)
	, memoryCardPathEdit(nullptr)
	, enableDebugLogCheck(nullptr)
	, m_config(config)
{
	setWindowTitle(tr("Preferences"));
	resize(700, 500);
	setupUI();
	loadCurrentSettings();
}

void SettingsDialog::setupUI()
{
	QHBoxLayout* mainLayout = new QHBoxLayout;
	mainLayout->setContentsMargins(0, 0, 0, 0);

	categoryList = new QListWidget;
	categoryList->setMaximumWidth(150);
	categoryList->addItem(tr("Interface"));
	categoryList->addItem(tr("Behavior"));
	categoryList->addItem(tr("Files"));
	categoryList->addItem(tr("Advanced"));
	categoryList->setCurrentRow(0);
	connect(categoryList, &QListWidget::itemClicked, this, &SettingsDialog::onCategorySelected);

	mainLayout->addWidget(categoryList);

	settingsStack = new QStackedWidget;

	createInterfaceCategory();
	createBehaviorCategory();
	createFilesCategory();
	createAdvancedCategory();

	mainLayout->addWidget(settingsStack, 1);

	QWidget* contentWidget = new QWidget;
	contentWidget->setLayout(mainLayout);

	QVBoxLayout* dialogLayout = new QVBoxLayout(this);
	dialogLayout->addWidget(contentWidget, 1);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(this, &QDialog::accepted, this, &SettingsDialog::onAccepted);

	dialogLayout->addWidget(buttonBox);
}

void SettingsDialog::createInterfaceCategory()
{
	QWidget* page = new QWidget;
	QVBoxLayout* layout = new QVBoxLayout(page);

	QLabel* headerLabel = new QLabel(tr("Interface Settings"));
	QFont headerFont = headerLabel->font();
	headerFont.setBold(true);
	headerFont.setPointSize(headerFont.pointSize() + 2);
	headerLabel->setFont(headerFont);
	headerLabel->setStyleSheet("color: #0078D4; padding: 10px 0px;");
	layout->addWidget(headerLabel);

	QGroupBox* appearanceGroup = new QGroupBox(tr("Appearance"), this);
	QVBoxLayout* appearanceLayout = new QVBoxLayout;

	darkModeCheck = new QCheckBox(tr("Enable Dark Mode (Restart required)"));
	appearanceLayout->addWidget(darkModeCheck);

	QHBoxLayout* thumbLayout = new QHBoxLayout;
	thumbLayout->addWidget(new QLabel(tr("Thumbnail Size:")));
	thumbnailSizeSpinner = new QSpinBox;
	thumbnailSizeSpinner->setMinimum(32);
	thumbnailSizeSpinner->setMaximum(256);
	thumbnailSizeSpinner->setSingleStep(16);
	thumbnailSizeSpinner->setValue(64);
	thumbLayout->addWidget(thumbnailSizeSpinner);
	thumbLayout->addWidget(new QLabel(tr("pixels")));
	thumbLayout->addStretch();
	appearanceLayout->addLayout(thumbLayout);

	appearanceGroup->setLayout(appearanceLayout);
	layout->addWidget(appearanceGroup);

	QGroupBox* renderGroup = new QGroupBox(tr("Rendering"), this);
	QVBoxLayout* renderLayout = new QVBoxLayout;

	QHBoxLayout* rendererLayout = new QHBoxLayout;
	rendererLayout->addWidget(new QLabel(tr("Renderer:")));
	rendererCombo = new QComboBox;
	rendererCombo->addItem(tr("Vulkan"), "vulkan");
	rendererCombo->addItem(tr("OpenGL"), "opengl");
	connect(rendererCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
		this, &SettingsDialog::onRendererChanged);
	rendererLayout->addWidget(rendererCombo);

	animateIconsCheck = new QCheckBox(tr("Animate Icons"));
	renderLayout->addWidget(animateIconsCheck);

	QHBoxLayout* lightingLayout = new QHBoxLayout;
	lightingLayout->addWidget(new QLabel(tr("Lighting:")));
	lightingCombo = new QComboBox;
	lightingCombo->addItem(tr("Icon Lighting"), "icon");
	lightingCombo->addItem(tr("Lighting Off"), "off");
	lightingCombo->addItem(tr("Alternate Lighting"), "alt1");
	lightingCombo->addItem(tr("Alternate Lighting 2"), "alt2");
	lightingLayout->addWidget(lightingCombo);
	lightingLayout->addStretch();
	renderLayout->addLayout(lightingLayout);

	QHBoxLayout* cameraLayout = new QHBoxLayout;
	cameraLayout->addWidget(new QLabel(tr("Camera:")));
	cameraCombo = new QComboBox;
	cameraCombo->addItem(tr("Camera Default"), "default");
	cameraCombo->addItem(tr("Camera Flat"), "flat");
	cameraCombo->addItem(tr("Camera Near"), "near");
	cameraCombo->addItem(tr("Camera High"), "high");
	cameraLayout->addWidget(cameraCombo);
	cameraLayout->addStretch();
	renderLayout->addLayout(cameraLayout);
	rendererLayout->addStretch();
	renderLayout->addLayout(rendererLayout);

	QLabel* rendererNote = new QLabel(tr("Note: Renderer will reload automatically."));
	rendererNote->setStyleSheet("color: #808080; font-size: 10px;");
	renderLayout->addWidget(rendererNote);

	renderGroup->setLayout(renderLayout);
	layout->addWidget(renderGroup);

	layout->addStretch();
	settingsStack->addWidget(page);
}

void SettingsDialog::createBehaviorCategory()
{
	QWidget* page = new QWidget;
	QVBoxLayout* layout = new QVBoxLayout(page);

	QLabel* headerLabel = new QLabel(tr("Behavior Settings"));
	QFont headerFont = headerLabel->font();
	headerFont.setBold(true);
	headerFont.setPointSize(headerFont.pointSize() + 2);
	headerLabel->setFont(headerFont);
	headerLabel->setStyleSheet("color: #0078D4; padding: 10px 0px;");
	layout->addWidget(headerLabel);

	QGroupBox* behaviorGroup = new QGroupBox(tr("General"), this);
	QVBoxLayout* behaviorLayout = new QVBoxLayout;

	warnOnDeleteCheck = new QCheckBox(tr("Warn before deleting files"));
	behaviorLayout->addWidget(warnOnDeleteCheck);

	confirmShutdownCheck = new QCheckBox(tr("Confirm shutdown"));
	behaviorLayout->addWidget(confirmShutdownCheck);

	hideToTrayCheck = new QCheckBox(tr("Hide to system tray when closing"));
	behaviorLayout->addWidget(hideToTrayCheck);

	behaviorGroup->setLayout(behaviorLayout);
	layout->addWidget(behaviorGroup);

	layout->addStretch();
	settingsStack->addWidget(page);
}

void SettingsDialog::createFilesCategory()
{
	QWidget* page = new QWidget;
	QVBoxLayout* layout = new QVBoxLayout(page);

	QLabel* headerLabel = new QLabel(tr("Files & Folders"));
	QFont headerFont = headerLabel->font();
	headerFont.setBold(true);
	headerFont.setPointSize(headerFont.pointSize() + 2);
	headerLabel->setFont(headerFont);
	headerLabel->setStyleSheet("color: #0078D4; padding: 10px 0px;");
	layout->addWidget(headerLabel);

	QGroupBox* pathsGroup = new QGroupBox(tr("Default Paths"), this);
	QVBoxLayout* pathsLayout = new QVBoxLayout;

	QLabel* mcLabel = new QLabel(tr("Memory Card Folder:"));
	pathsLayout->addWidget(mcLabel);

	QHBoxLayout* mcPathLayout = new QHBoxLayout;
	memoryCardPathEdit = new QLineEdit;
	memoryCardPathEdit->setReadOnly(true);
	mcPathLayout->addWidget(memoryCardPathEdit);

	QPushButton* browseButton = new QPushButton(tr("Browse..."));
	connect(browseButton, &QPushButton::clicked, this, &SettingsDialog::onBrowseMemoryCardPath);
	mcPathLayout->addWidget(browseButton);

	pathsLayout->addLayout(mcPathLayout);
	pathsGroup->setLayout(pathsLayout);
	layout->addWidget(pathsGroup);

	layout->addStretch();
	settingsStack->addWidget(page);
}

void SettingsDialog::createAdvancedCategory()
{
	QWidget* page = new QWidget;
	QVBoxLayout* layout = new QVBoxLayout(page);

	QLabel* headerLabel = new QLabel(tr("Advanced Settings"));
	QFont headerFont = headerLabel->font();
	headerFont.setBold(true);
	headerFont.setPointSize(headerFont.pointSize() + 2);
	headerLabel->setFont(headerFont);
	headerLabel->setStyleSheet("color: #0078D4; padding: 10px 0px;");
	layout->addWidget(headerLabel);

	QGroupBox* debugGroup = new QGroupBox(tr("Debug"), this);
	QVBoxLayout* debugLayout = new QVBoxLayout;

	enableDebugLogCheck = new QCheckBox(tr("Enable debug logging"));
	debugLayout->addWidget(enableDebugLogCheck);

	debugGroup->setLayout(debugLayout);
	layout->addWidget(debugGroup);

	layout->addStretch();
	settingsStack->addWidget(page);
}

void SettingsDialog::onCategorySelected(QListWidgetItem* item)
{
	int index = categoryList->row(item);
	settingsStack->setCurrentIndex(index);
}

void SettingsDialog::loadCurrentSettings()
{
	if (!m_config)
		return;

	darkModeCheck->setChecked(m_config->getDarkMode());
	thumbnailSizeSpinner->setValue(m_config->getThumbnailSize());

	std::string renderer_str = m_config->getRenderer();
	QString rendererKey = QString::fromStdString(renderer_str).toLower();

	int rendererIndex = rendererCombo->findData(rendererKey);
	if (rendererIndex >= 0)
		rendererCombo->setCurrentIndex(rendererIndex);

	animateIconsCheck->setChecked(m_config->getAnimateIcons());

	const QString lighting = QString::fromStdString(m_config->getLightingMode()).toLower();
	int lightingIndex = lightingCombo->findData(lighting);
	if (lightingIndex >= 0)
		lightingCombo->setCurrentIndex(lightingIndex);

	const QString camera = QString::fromStdString(m_config->getCameraMode()).toLower();
	int cameraIndex = cameraCombo->findData(camera);
	if (cameraIndex >= 0)
		cameraCombo->setCurrentIndex(cameraIndex);

	warnOnDeleteCheck->setChecked(m_config->getWarnOnDelete());
	confirmShutdownCheck->setChecked(m_config->getConfirmShutdown());
	hideToTrayCheck->setChecked(m_config->getHideToTrayOnClose());
	memoryCardPathEdit->setText(QString::fromStdString(m_config->getMemoryCardFolder()));
	enableDebugLogCheck->setChecked(m_config->getDebugLogging());
}

void SettingsDialog::onAccepted()
{
	if (!m_config)
	{
		Logger::error("[SettingsDialog] No config object!");
		return;
	}

	m_config->setDarkMode(darkModeCheck->isChecked());
	m_config->setThumbnailSize(thumbnailSizeSpinner->value());

	QString renderer = rendererCombo->currentData().toString();
	m_config->setRenderer(renderer.toLower().toStdString());
	m_config->setAnimateIcons(animateIconsCheck->isChecked());
	m_config->setLightingMode(lightingCombo->currentData().toString().toStdString());
	m_config->setCameraMode(cameraCombo->currentData().toString().toStdString());

	m_config->setWarnOnDelete(warnOnDeleteCheck->isChecked());
	m_config->setConfirmShutdown(confirmShutdownCheck->isChecked());
	m_config->setHideToTrayOnClose(hideToTrayCheck->isChecked());
	m_config->setMemoryCardFolder(memoryCardPathEdit->text().toStdString());
	m_config->setDebugLogging(enableDebugLogCheck->isChecked());

	m_config->save();
}

void SettingsDialog::onBrowseMemoryCardPath()
{
	QString dir = QFileDialog::getExistingDirectory(this,
		tr("Select Memory Card Folder"), memoryCardPathEdit->text());
	if (!dir.isEmpty())
	{
		memoryCardPathEdit->setText(dir);
	}
}

void SettingsDialog::onRendererChanged(const QString& renderer)
{
	Logger::info("Renderer selected: {}", renderer.toStdString());
}
