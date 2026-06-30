// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#if defined(__linux__)

#include "GLContextEGL.h"

#include <EGL/egl.h>
#include <wayland-egl.h>
#include <wayland-client.h>
#include "Logger.h"
#include "../../../common/Error.h"
#include <cstring>

GLContextEGL::GLContextEGL(const WindowInfo& windowInfo)
	: GLContext(windowInfo)
	, m_display(EGL_NO_DISPLAY)
	, m_context(EGL_NO_CONTEXT)
	, m_surface(EGL_NO_SURFACE)
	, m_config(nullptr)
	, m_initialized(false)
{
}

GLContextEGL::~GLContextEGL()
{
	cleanup();
}

std::unique_ptr<GLContext> GLContextEGL::Create(const WindowInfo& windowInfo, Error* error)
{
	std::unique_ptr<GLContextEGL> context(new GLContextEGL(windowInfo));

	if (!context)
	{
		if (error)
			*error = Error::CreateString("GL: Failed to allocate GLContextEGL");
		return nullptr;
	}

	return context;
}

bool GLContextEGL::initialize()
{
	if (m_initialized)
		return true;

	if (!initializeDisplay())
		return false;

	if (!chooseConfig())
	{
		cleanup();
		return false;
	}

	if (!createSurface())
	{
		cleanup();
		return false;
	}

	if (!createContext())
	{
		cleanup();
		return false;
	}

	m_initialized = true;
	Logger::info("GL: Context initialized successfully");
	return true;
}

bool GLContextEGL::initializeDisplay()
{
	if (m_windowInfo.display_connection)
	{
		m_display = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(m_windowInfo.display_connection));
	}
	else
	{
		m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	}

	if (m_display == EGL_NO_DISPLAY)
		return failCreation("GL: Failed to get EGL display");

	EGLint major, minor;
	if (!eglInitialize(m_display, &major, &minor))
	{
		failCreation("GL: Failed to initialize EGL (EGL error " + std::to_string(eglGetError()) + ")");
		m_display = EGL_NO_DISPLAY;
		return false;
	}

	Logger::info("GL: EGL initialized: version {}.{}", major, minor);
	return true;
}

bool GLContextEGL::chooseConfig()
{
	const EGLint configAttribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE};

	EGLint numConfigs;
	if (!eglChooseConfig(m_display, configAttribs, &m_config, 1, &numConfigs) || numConfigs == 0)
		return failCreation("GL: Failed to choose EGL config (EGL error " + std::to_string(eglGetError()) + ")");

	Logger::info("GL: EGL config chosen");
	return true;
}

bool GLContextEGL::createSurface()
{
	if (!m_windowInfo.window_handle)
		return failCreation("GL: No native window handle provided");

	EGLNativeWindowType nativeWindow = reinterpret_cast<EGLNativeWindowType>(m_windowInfo.window_handle);

	if (m_windowInfo.type == WindowInfo::Type::Wayland)
	{
		struct wl_surface* surf = reinterpret_cast<struct wl_surface*>(m_windowInfo.window_handle);
		m_waylandWindow = wl_egl_window_create(surf, m_width, m_height);
		if (!m_waylandWindow)
			return failCreation("GL: Failed to create wl_egl_window");
		nativeWindow = reinterpret_cast<EGLNativeWindowType>(m_waylandWindow);
	}

	m_surface = eglCreateWindowSurface(m_display, m_config, nativeWindow, nullptr);

	if (m_surface == EGL_NO_SURFACE)
		return failCreation("GL: Failed to create EGL surface (EGL error " + std::to_string(eglGetError()) + ")");

	Logger::info("GL: EGL surface created");
	return true;
}

bool GLContextEGL::createContext()
{
	if (!eglBindAPI(EGL_OPENGL_API))
		return failCreation("GL: Failed to bind OpenGL API (EGL error " + std::to_string(eglGetError()) + ")");

	const EGLint contextAttribs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 3,
		EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
		EGL_NONE};

	m_context = eglCreateContext(m_display, m_config, EGL_NO_CONTEXT, contextAttribs);
	if (m_context == EGL_NO_CONTEXT)
		return failCreation("GL: Failed to create EGL context (EGL error " + std::to_string(eglGetError()) + ")");

	Logger::info("GL: EGL context created");
	return true;
}

bool GLContextEGL::makeCurrent()
{
	if (m_display == EGL_NO_DISPLAY || m_context == EGL_NO_CONTEXT || m_surface == EGL_NO_SURFACE)
		return false;

	if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context))
		return failCreation("GL: Failed to make context current (EGL error " + std::to_string(eglGetError()) + ")");

	return true;
}

void GLContextEGL::releaseCurrent()
{
	if (m_display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	}
}

void GLContextEGL::swapBuffers()
{
	if (m_display != EGL_NO_DISPLAY && m_surface != EGL_NO_SURFACE)
	{
		eglSwapBuffers(m_display, m_surface);
	}
}

void GLContextEGL::resize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
	if (m_waylandWindow)
	{
		wl_egl_window_resize(m_waylandWindow, width, height, 0, 0);
	}
}

void* GLContextEGL::getProcAddress(const char* name)
{
	return reinterpret_cast<void*>(eglGetProcAddress(name));
}

void GLContextEGL::setVSync(bool enabled)
{
	try
	{
		if (m_display != EGL_NO_DISPLAY && m_surface != EGL_NO_SURFACE)
		{
			EGLint interval = enabled ? 1 : 0;
			if (eglSwapInterval(m_display, interval))
			{
				Logger::info("GL: VSync {}", enabled ? "enabled" : "disabled");
			}
			else
			{
				Logger::error("GL: Failed to set VSync interval: {}", eglGetError());
			}
		}
		else
		{
			Logger::warn("GL: Cannot set VSync display or surface not available");
		}
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Exception setting VSync: {}", e.what());
	}
}

void GLContextEGL::cleanup()
{
	releaseCurrent();

	if (m_display != EGL_NO_DISPLAY)
	{
		if (m_context != EGL_NO_CONTEXT)
		{
			eglDestroyContext(m_display, m_context);
			m_context = EGL_NO_CONTEXT;
		}

		if (m_surface != EGL_NO_SURFACE)
		{
			eglDestroySurface(m_display, m_surface);
			m_surface = EGL_NO_SURFACE;
		}

		eglTerminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}

	if (m_waylandWindow)
	{
		wl_egl_window_destroy(m_waylandWindow);
		m_waylandWindow = nullptr;
	}

	m_initialized = false;
	Logger::info("GL: Cleanup complete");
}

#endif // __linux__
