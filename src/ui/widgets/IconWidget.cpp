// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "IconWidget.h"
#include "ui_IconWidget.h"
#include "common/Config.h"
#include "common/Logger.h"
#include "common/Error.h"
#include "common/WindowInfo.h"
#include "core/renderer/Renderer.h"
#include <QResizeEvent>
#include <QWindow>
#if defined(__linux__)
#include <qpa/qplatformnativeinterface.h>
#endif

namespace
{

	std::string toLowerCopy(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		return value;
	}

	LightingMode parseLightingMode(const std::string& value)
	{
		const std::string v = toLowerCopy(value);
		if (v == "off")
			return LightingMode::Off;
		if (v == "alt1" || v == "alternate" || v == "alternate_lighting")
			return LightingMode::Alternate1;
		if (v == "alt2" || v == "alternate2")
			return LightingMode::Alternate2;
		return LightingMode::Icon;
	}

	CameraMode parseCameraMode(const std::string& value)
	{
		const std::string v = toLowerCopy(value);
		if (v == "flat")
			return CameraMode::Flat;
		if (v == "near")
			return CameraMode::Near;
		if (v == "high")
			return CameraMode::High;
		return CameraMode::Default;
	}

} // namespace

IconWidget::IconWidget(Config* config, QWidget* parent)
	: QWidget(parent)
	, m_config(config)
	, m_renderWindow(new QWindow())
{
	Ui::IconWidget ui;
	ui.setupUi(this);

	m_renderWindow->installEventFilter(this);
	QWidget* windowContainer = QWidget::createWindowContainer(m_renderWindow, this);
	windowContainer->setFocusPolicy(Qt::NoFocus);
	ui.renderLayout->addWidget(windowContainer);
	setFocusPolicy(Qt::NoFocus);
}

IconWidget::~IconWidget()
{
	stopRendering();
	if (m_renderer)
	{
		m_renderer->shutdown();
		m_renderer.reset();
	}
	m_windowHandle = nullptr;
}

bool IconWidget::loadIcon(const std::vector<uint8_t>& iconData)
{
	m_icon = std::make_shared<PS2Icon::Icon>();
	if (!m_icon->load(iconData))
	{
		const QString err = QString::fromStdString(m_icon->getError());
		m_icon.reset();
		Logger::warn("IconWidget: Failed to load icon: {}", err.toStdString());
		emit iconLoadFailed(err);
		return false;
	}

	if (!ensureRenderer())
		return false;

	m_renderer->setIcon(m_icon);

	emit iconLoaded();

	bool animate = true;
	if (m_config)
	{
		animate = m_config->getAnimateIcons();
	}

	if (animate)
	{
		startRendering();
	}
	else
	{
		stopRendering();
		requestRender();
	}

	return true;
}

bool IconWidget::loadIcon(const std::string& iconData)
{
	std::vector<uint8_t> bytes(iconData.begin(), iconData.end());
	return loadIcon(bytes);
}

void IconWidget::setAnimationEnabled(bool enabled)
{
	if (!ensureRenderer())
		return;

	m_renderer->setAnimationEnabled(enabled);
	if (enabled)
	{
		startRendering();
	}
	else
	{
		stopRendering();
		requestRender();
	}
}

bool IconWidget::isAnimationEnabled() const
{
	if (m_renderer)
		return m_renderer->isAnimationEnabled();
	return false;
}

void IconWidget::setRotation(float x, float y, float z)
{
	m_rotX = x;
	m_rotY = y;
	ensureRenderer();
	if (m_renderer)
	{
		m_renderer->setRotation(x, y, z);
		requestRender();
	}
}

void IconWidget::setZoom(float zoom)
{
	m_zoom = std::clamp(zoom, 0.1f, 10.0f);
	ensureRenderer();
	if (m_renderer)
	{
		m_renderer->setZoom(m_zoom);
		requestRender();
	}
}

void IconWidget::resetCamera()
{
	m_rotX = 0.0f;
	m_rotY = 0.0f;
	m_zoom = 1.0f;
	ensureRenderer();
	if (m_renderer)
	{
		m_renderer->resetCamera();
		requestRender();
	}
}

void IconWidget::zoomIn()
{
	setZoom(m_zoom - 0.2f);
}

void IconWidget::zoomOut()
{
	setZoom(m_zoom + 0.2f);
}

void IconWidget::setLightingFromIconSys(PS2IconSys* iconSys)
{
	ensureRenderer();
	if (m_renderer)
	{
		m_renderer->setLightingFromIconSys(iconSys);
		requestRender();
	}
}

void IconWidget::setBackgroundFromIconSys(PS2IconSys* iconSys)
{
	ensureRenderer();
	if (m_renderer)
	{
		m_renderer->setBackgroundFromIconSys(iconSys);
		requestRender();
	}
}

void IconWidget::setBackgroundColor(float r, float g, float b, float a)
{
	ensureRenderer();
	if (m_renderer)
	{
		m_renderer->setBackgroundColor(r, g, b, a);
		requestRender();
	}
}

void IconWidget::applyConfigToRenderer(PS2IconSys* iconSys)
{
	ensureRenderer();
	if (!m_renderer)
		return;

	const bool animate = m_config ? m_config->getAnimateIcons() : true;
	const LightingMode lightingMode = parseLightingMode(m_config ? m_config->getLightingMode() : "icon");
	const CameraMode cameraMode = parseCameraMode(m_config ? m_config->getCameraMode() : "default");

	m_renderer->setLightingMode(lightingMode);
	m_renderer->setLightingFromIconSys(iconSys);
	m_renderer->setCameraMode(cameraMode);
	m_renderer->setAnimationEnabled(animate);

	if (animate)
	{
		startRendering();
	}
	else
	{
		stopRendering();
		requestRender();
	}
}

void IconWidget::startRendering()
{
	if (m_renderTimer.isActive())
		return;

	int maxFps = m_config ? m_config->getMaxFPS() : 30;
	int interval;

	if (maxFps <= 0)
	{
		interval = 0;
	}
	else
	{
		interval = static_cast<int>(1000.0f / static_cast<float>(maxFps));
		if (interval <= 0)
			interval = 1;
	}

	m_renderTimer.start(interval, this);
	requestRender();
}

void IconWidget::stopRendering()
{
	if (m_renderTimer.isActive())
	{
		m_renderTimer.stop();
	}
}

bool IconWidget::ensureRenderer()
{
	void* currentHandle = nullptr;
#if defined(__linux__)
	if (QGuiApplication::platformName().startsWith("wayland", Qt::CaseInsensitive))
	{
		QPlatformNativeInterface* pni = QGuiApplication::platformNativeInterface();
		if (pni)
			currentHandle = pni->nativeResourceForWindow("surface", m_renderWindow);
	}
	else
	{
		currentHandle = reinterpret_cast<void*>(m_renderWindow->winId());
	}
#else
	currentHandle = reinterpret_cast<void*>(m_renderWindow->winId());
#endif

	if (m_renderer)
	{
		if (m_windowHandle == currentHandle && currentHandle != nullptr)
			return true;

		m_renderer->shutdown();
		m_renderer.reset();
	}

	m_renderWindow->create();

	const QSize windowSize = m_renderWindow->size().isEmpty() ? QSize(256, 256) : m_renderWindow->size();

#if defined(__APPLE__)
	const std::string rendererType = m_config ? m_config->getRenderer() : "metal";
#else
	const std::string rendererType = m_config ? m_config->getRenderer() : "vulkan";
#endif

	WindowInfo wi{};
	const qreal dpr = m_renderWindow->devicePixelRatio();
	wi.surface_width = static_cast<uint32_t>(windowSize.width() * dpr);
	wi.surface_height = static_cast<uint32_t>(windowSize.height() * dpr);
	wi.surface_scale = static_cast<float>(dpr);

#if defined(_WIN32)
	wi.type = WindowInfo::Type::Win32;
	wi.window_handle = reinterpret_cast<void*>(m_renderWindow->winId());
#elif defined(__APPLE__)
	wi.type = WindowInfo::Type::MacOS;
	wi.window_handle = reinterpret_cast<void*>(m_renderWindow->winId());
#elif defined(__linux__)
	if (QGuiApplication::platformName().startsWith("wayland", Qt::CaseInsensitive))
	{
		wi.type = WindowInfo::Type::Wayland;

		auto* waylandApp = qApp->nativeInterface<QNativeInterface::QWaylandApplication>();
		if (waylandApp)
		{
			wi.display_connection = waylandApp->display();
		}

		QPlatformNativeInterface* pni = QGuiApplication::platformNativeInterface();
		if (pni)
		{
			if (!wi.display_connection)
				wi.display_connection = pni->nativeResourceForWindow("display", nullptr);
			wi.window_handle = pni->nativeResourceForWindow("surface", m_renderWindow);
		}

		if (!wi.window_handle)
		{
			Logger::info("IconWidget: Wayland surface not ready yet");
			if (m_icon)
				emit iconLoadFailed(tr("Wayland surface is not ready yet"));
			return false;
		}
	}
	else if (QGuiApplication::platformName().startsWith("xcb", Qt::CaseInsensitive))
	{
		wi.type = WindowInfo::Type::X11;

		auto* x11App = qApp->nativeInterface<QNativeInterface::QX11Application>();
		if (x11App)
		{
			wi.display_connection = x11App->display();
		}
		wi.window_handle = reinterpret_cast<void*>(m_renderWindow->winId());
	}
#endif

	if (!wi.window_handle && wi.type != WindowInfo::Type::Surfaceless)
	{
		Logger::error("IconWidget: Invalid window handle, postponing renderer creation");
		if (m_icon)
			emit iconLoadFailed(tr("Native window is not available yet"));
		return false;
	}

	Logger::info("IconWidget: Creating renderer: {}", rendererType);

	Error rendererError;

	if (rendererType == "opengl")
	{
		m_renderer = RendererFactory::createOpenGLRenderer(wi, m_config, &rendererError);
	}
#if defined(__APPLE__)
	else if (rendererType == "metal")
	{
		m_renderer = RendererFactory::createMetalRenderer(wi, m_config, &rendererError);
	}
#endif
	else
	{
#if defined(ENABLE_VULKAN)
		m_renderer = RendererFactory::createVulkanRenderer(wi, m_config, &rendererError);
		if (!m_renderer)
		{
			Logger::warn("IconWidget: Vulkan initialization failed, falling back to OpenGL: {}",
				rendererError.IsValid() ? rendererError.GetDescription() : "Unknown error");
			rendererError.Clear();
			m_renderer = RendererFactory::createOpenGLRenderer(wi, m_config, &rendererError);
		}
#else
		m_renderer = RendererFactory::createOpenGLRenderer(wi, m_config, &rendererError);
#endif
	}

	if (!m_renderer)
	{
		if (m_icon)
		{
			const QString msg = rendererError.IsValid() ? QString::fromStdString(rendererError.GetDescription()) : tr("Failed to initialize renderer");
			emit iconLoadFailed(msg);
		}
		return false;
	}

	m_windowHandle = wi.window_handle;

	if (m_icon)
	{
		m_renderer->setIcon(m_icon);
	}
	m_renderer->setRotation(m_rotX, m_rotY, 0.0f);
	m_renderer->setZoom(m_zoom);

	return true;
}

void IconWidget::recreateRenderer()
{
	if (m_renderer)
	{
		m_renderer->shutdown();
		m_renderer.reset();
	}
	m_windowHandle = nullptr;

	ensureRenderer();
}

void IconWidget::requestRender()
{
	if (m_renderWindow->isExposed())
		m_renderWindow->requestUpdate();
}

void IconWidget::renderFrame()
{
	if (!m_renderWindow->isExposed())
		return;

	ensureRenderer();
	if (!m_renderer)
		return;

	if (!m_renderer->isInitialized())
	{
		recreateRenderer();
		if (!m_renderer || !m_renderer->isInitialized())
			return;
	}

	m_renderer->render();
}

QSize IconWidget::sizeHint() const
{
	return QSize(256, 256);
}

bool IconWidget::hasHeightForWidth() const
{
	return true;
}

int IconWidget::heightForWidth(int w) const
{
	return w;
}

bool IconWidget::eventFilter(QObject* watched, QEvent* event)
{
	if (watched != m_renderWindow)
		return QWidget::eventFilter(watched, event);

	switch (event->type())
	{
		case QEvent::UpdateRequest:
			renderFrame();
			return true;

		case QEvent::Expose:
			requestRender();
			break;

		case QEvent::Resize:
		{
			if (!m_renderer)
				break;

			const QSize windowSize = static_cast<QResizeEvent*>(event)->size();
			if (windowSize.isEmpty())
				break;

			const qreal dpr = m_renderWindow->devicePixelRatio();
			m_renderer->resize(
				static_cast<uint32_t>(windowSize.width() * dpr),
				static_cast<uint32_t>(windowSize.height() * dpr));
			requestRender();
			break;
		}

		case QEvent::MouseButtonPress:
		{
			auto* mouseEvent = static_cast<QMouseEvent*>(event);
			if (mouseEvent->button() == Qt::LeftButton)
			{
				m_dragging = true;
				m_lastDragPos = mouseEvent->position().toPoint();
			}
			break;
		}

		case QEvent::MouseMove:
		{
			auto* mouseEvent = static_cast<QMouseEvent*>(event);
			if (m_dragging && (mouseEvent->buttons() & Qt::LeftButton))
			{
				const QPoint position = mouseEvent->position().toPoint();
				const QPoint delta = position - m_lastDragPos;
				m_lastDragPos = position;
				m_rotY += static_cast<float>(delta.x()) * 0.005f;
				m_rotX += static_cast<float>(delta.y()) * 0.005f;

				if (ensureRenderer())
				{
					m_renderer->setRotation(m_rotX, m_rotY, 0.0f);
					requestRender();
				}
			}
			break;
		}

		case QEvent::MouseButtonRelease:
		{
			auto* mouseEvent = static_cast<QMouseEvent*>(event);
			if (mouseEvent->button() == Qt::LeftButton)
				m_dragging = false;
			break;
		}

		case QEvent::Wheel:
		{
			auto* wheelEvent = static_cast<QWheelEvent*>(event);
			const float steps = static_cast<float>(wheelEvent->angleDelta().y()) / 120.0f;
			setZoom(m_zoom - steps * 0.15f);
			wheelEvent->accept();
			return true;
		}

		default:
			break;
	}

	return QWidget::eventFilter(watched, event);
}

void IconWidget::timerEvent(QTimerEvent* event)
{
	if (event->timerId() == m_renderTimer.timerId())
	{
		requestRender();
		return;
	}

	QWidget::timerEvent(event);
}

void IconWidget::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	ensureRenderer();
	if (m_icon && m_renderer && m_renderer->isAnimationEnabled())
		startRendering();
	else if (m_icon)
		requestRender();
}

void IconWidget::hideEvent(QHideEvent* event)
{
	QWidget::hideEvent(event);
	stopRendering();
}
