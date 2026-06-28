// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "IconWidget.h"
#include "Config.h"
#include "../../common/Logger.h"
#include "../common/WindowInfo.h"
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#if defined(__linux__)
#include <qpa/qplatformnativeinterface.h>
#endif
#include <algorithm>
#include <cctype>

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
		const std::string Sv = toLowerCopy(value);
		if (Sv == "flat")
			return CameraMode::Flat;
		if (Sv == "near")
			return CameraMode::Near;
		if (Sv == "high")
			return CameraMode::High;
		return CameraMode::Default;
	}

} // namespace

static float getRefreshRate(QWidget* widget)
{
	WindowInfo wi{};
#if defined(_WIN32)
	wi.type = WindowInfo::Type::Win32;
	wi.window_handle = reinterpret_cast<void*>(widget->winId());
#elif defined(__linux__)
	if (QGuiApplication::platformName().startsWith("wayland", Qt::CaseInsensitive))
	{
		wi.type = WindowInfo::Type::Wayland;
		if (widget->windowHandle())
		{
			QPlatformNativeInterface* pni = QGuiApplication::platformNativeInterface();
			if (pni)
			{
				wi.display_connection = pni->nativeResourceForWindow("display", widget->windowHandle());
				wi.window_handle = pni->nativeResourceForWindow("surface", widget->windowHandle());
			}
		}
	}
	else
	{
		wi.type = WindowInfo::Type::X11;
		auto* x11App = qApp->nativeInterface<QNativeInterface::QX11Application>();
		if (x11App)
		{
			wi.display_connection = x11App->display();
		}
		wi.window_handle = reinterpret_cast<void*>(widget->winId());
	}
#endif

	auto rate = WindowInfo::QueryRefreshRateForWindow(wi);
	if (rate.has_value() && *rate > 0.0f)
		return *rate;

	return 60.0f;
}

IconWidget::IconWidget(Config* config, QWidget* parent)
	: QWidget(parent)
	, m_config(config)
{
	setAttribute(Qt::WA_NativeWindow);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_DontCreateNativeAncestors, true);
	setFocusPolicy(Qt::NoFocus);
	printPlatformInfo();
	Logger::info("IconWidget constructed: {}", (void*)this);
}

IconWidget::~IconWidget()
{
	Logger::info("IconWidget destructor start: {}", (void*)this);
	stopRendering();
	if (m_renderer)
	{
		Logger::info("IconWidget shutting down renderer");
		m_renderer->shutdown();
		m_renderer.reset();
	}
	Logger::info("IconWidget destructor end: {}", (void*)this);
}

void IconWidget::printPlatformInfo()
{
	QString platform = QGuiApplication::platformName();
	qInfo() << "IconWidget: Platform:" << platform;
}

bool IconWidget::loadIcon(const std::vector<uint8_t>& iconData)
{
	try
	{
		m_icon = std::make_shared<PS2Icon::Icon>();
		if (!m_icon->load(iconData))
		{
			QString err = QString::fromStdString(m_icon->getError());
			m_icon.reset();
			emit iconLoadFailed(err);
			return false;
		}

		ensureRenderer();
		if (m_renderer)
		{
			m_renderer->setIcon(m_icon);
		}

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
			renderFrame();
		}

		return true;
	}
	catch (const std::exception& e)
	{
		emit iconLoadFailed(QString::fromStdString(e.what()));
		return false;
	}
}

bool IconWidget::loadIcon(const std::string& iconData)
{
	std::vector<uint8_t> bytes(iconData.begin(), iconData.end());
	return loadIcon(bytes);
}

void IconWidget::setAnimationEnabled(bool enabled)
{
	ensureRenderer();
	if (m_renderer)
	{
		m_renderer->setAnimationEnabled(enabled);
		if (enabled)
			startRendering();
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
		renderFrame();
	}
}

void IconWidget::setZoom(float zoom)
{
	m_zoom = std::clamp(zoom, 0.1f, 10.0f);
	ensureRenderer();
	if (m_renderer)
	{
		m_renderer->setZoom(m_zoom);
		renderFrame();
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
		renderFrame();
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
		renderFrame();
	}
}

void IconWidget::setBackgroundFromIconSys(PS2IconSys* iconSys)
{
	ensureRenderer();
	if (m_renderer)
	{
		m_renderer->setBackgroundFromIconSys(iconSys);
		renderFrame();
	}
}

void IconWidget::setBackgroundColor(float r, float g, float b, float a)
{
	ensureRenderer();
	if (m_renderer)
	{
		m_renderer->setBackgroundColor(r, g, b, a);
		renderFrame();
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
		renderFrame();
	}
}

void IconWidget::startRendering()
{
	if (m_renderLoopEnabled && m_renderTimer.isActive())
		return;

	Logger::info("IconWidget::startRendering {}", (void*)this);
	m_renderLoopEnabled = true;

	int maxFps = m_config ? m_config->getMaxFPS() : 30;
	float rate;

	if (maxFps <= 0)
	{
		rate = getRefreshRate(this);
		if (rate < 30.0f)
			rate = 60.0f;
	}
	else
	{
		rate = static_cast<float>(maxFps);
	}

	int interval = static_cast<int>(1000.0f / rate);
	if (interval <= 0)
		interval = 16;

	m_renderTimer.start(interval, this);
	renderFrame();
}

void IconWidget::stopRendering()
{
	Logger::info("IconWidget::stopRendering {}", (void*)this);
	m_renderLoopEnabled = false;
	if (m_renderTimer.isActive())
	{
		m_renderTimer.stop();
	}
}

void IconWidget::ensureRenderer()
{
	if (m_renderer)
		return;

	winId();

	const QSize s = size().isEmpty() ? QSize(256, 256) : size();
	try
	{
#if defined(__APPLE__)
		const std::string rendererType = m_config ? m_config->getRenderer() : "metal";
#else
		const std::string rendererType = m_config ? m_config->getRenderer() : "vulkan";
#endif

		WindowInfo wi{};
		const qreal dpr = devicePixelRatioF();
		wi.surface_width = static_cast<uint32_t>(s.width() * dpr);
		wi.surface_height = static_cast<uint32_t>(s.height() * dpr);
		wi.surface_scale = static_cast<float>(dpr);

#if defined(_WIN32)
		wi.type = WindowInfo::Type::Win32;
		wi.window_handle = reinterpret_cast<void*>(winId());
#elif defined(__APPLE__)
		wi.type = WindowInfo::Type::MacOS;
		wi.window_handle = reinterpret_cast<void*>(winId());
#elif defined(__linux__)
		if (QGuiApplication::platformName().startsWith("wayland", Qt::CaseInsensitive))
		{
			wi.type = WindowInfo::Type::Wayland;
			QPlatformNativeInterface* pni = QGuiApplication::platformNativeInterface();
			if (pni)
			{
				if (windowHandle())
				{
					wi.display_connection = pni->nativeResourceForWindow("display", windowHandle());
					wi.window_handle = pni->nativeResourceForWindow("surface", windowHandle());
				}
			}

			if (!wi.window_handle)
			{
				Logger::info("IconWidget: Wayland surface not ready yet");
				return;
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
			wi.window_handle = reinterpret_cast<void*>(winId());
		}
#endif

		if (!wi.window_handle && wi.type != WindowInfo::Type::Surfaceless)
		{
			Logger::error("IconWidget: Invalid window handle, postponing renderer creation");
			return;
		}

		Logger::info("IconWidget: Creating renderer: {}", rendererType);

		if (rendererType == "opengl")
		{
			m_renderer = RendererFactory::createOpenGLRenderer(wi, m_config);
		}
#if defined(__APPLE__)
		else if (rendererType == "metal")
		{
			m_renderer = RendererFactory::createMetalRenderer(wi, m_config);
		}
#endif
		else
		{
#if defined(ENABLE_VULKAN)
			m_renderer = RendererFactory::createVulkanRenderer(wi, m_config);
#endif
		}

		if (!m_renderer)
		{
			qWarning() << "IconWidget: renderer creation failed";
			return;
		}

		if (m_icon)
		{
			m_renderer->setIcon(m_icon);
		}
	}
	catch (const std::exception& e)
	{
		qWarning() << "IconWidget: exception creating renderer:" << e.what();
		m_renderer.reset();
	}
}

void IconWidget::recreateRenderer()
{
	if (m_renderer)
	{
		m_renderer->shutdown();
		m_renderer.reset();
	}

	ensureRenderer();

	if (m_renderer && m_icon)
	{
		m_renderer->setIcon(m_icon);
	}
}

void IconWidget::renderFrame()
{
	ensureRenderer();
	if (!m_renderer)
		return;

	try
	{
		if (!m_renderer->isInitialized())
		{
			recreateRenderer();
			if (!m_renderer || !m_renderer->isInitialized())
				return;
		}

		m_renderer->render();
	}
	catch (const std::exception& e)
	{
		qWarning() << "IconWidget: render exception, recreating renderer:" << e.what();
		recreateRenderer();
	}
}

void IconWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_dragging = true;
		m_lastDragPos = event->pos();
	}
	QWidget::mousePressEvent(event);
}

void IconWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (m_dragging && (event->buttons() & Qt::LeftButton))
	{
		const QPoint delta = event->pos() - m_lastDragPos;
		m_lastDragPos = event->pos();
		m_rotY += static_cast<float>(delta.x()) * 0.005f;
		m_rotX += static_cast<float>(delta.y()) * 0.005f;
		ensureRenderer();
		if (m_renderer)
		{
			m_renderer->setRotation(m_rotX, m_rotY, 0.0f);
			renderFrame();
		}
	}
	QWidget::mouseMoveEvent(event);
}

void IconWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
		m_dragging = false;
	QWidget::mouseReleaseEvent(event);
}

void IconWidget::wheelEvent(QWheelEvent* event)
{
	const float steps = static_cast<float>(event->angleDelta().y()) / 120.0f;
	setZoom(m_zoom - steps * 0.15f);
	event->accept();
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

void IconWidget::timerEvent(QTimerEvent* event)
{
	if (event->timerId() == m_renderTimer.timerId())
	{
		if (m_renderLoopEnabled)
		{
			renderFrame();
		}
		return;
	}

	QWidget::timerEvent(event);
}

void IconWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	if (event->size().isEmpty())
		return;

	if (m_renderer)
	{
		const qreal dpr = devicePixelRatioF();
		m_renderer->resize(
			static_cast<uint32_t>(event->size().width() * dpr),
			static_cast<uint32_t>(event->size().height() * dpr));
		renderFrame();
	}
}

void IconWidget::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	ensureRenderer();
	if (m_icon)
	{
		startRendering();
	}
}

QPaintEngine* IconWidget::paintEngine() const
{
	return nullptr;
}
