// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#ifdef _WIN32

#include "GLContextWGL.h"
#define NOMINMAX
#include <windows.h>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif
#include <glad/gl.h>
#include <glad/wgl.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include "Logger.h"
#include <cstring>

extern "C" {
typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);
}

static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;

static const wchar_t* g_windowClassName = L"GLContextWGL_Hidden";

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CREATE)
	{
		GLContextWGL* pThis = nullptr;
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		if (pCreate)
		{
			pThis = reinterpret_cast<GLContextWGL*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

GLContextWGL::GLContextWGL(const WindowInfo& windowInfo)
	: GLContext(windowInfo)
	, m_hwnd(nullptr)
	, m_hdc(nullptr)
	, m_hglrc(nullptr)
	, m_hinstance(GetModuleHandle(nullptr))
	, m_ownsWindow(windowInfo.window_handle == nullptr)
	, m_initialized(false)
{
}

GLContextWGL::~GLContextWGL()
{
	cleanup();
}

std::unique_ptr<GLContext> GLContextWGL::Create(const WindowInfo& windowInfo, std::string* error)
{
	std::unique_ptr<GLContextWGL> context(new GLContextWGL(windowInfo));

	if (!context)
	{
		if (error)
			*error = "Failed to allocate GLContextWGL";
		return nullptr;
	}

	return context;
}

bool GLContextWGL::initialize()
{
	try
	{
		if (m_initialized)
			return true;

		if (!createWindow())
		{
			Logger::error("GL: Failed to create window");
			return false;
		}

		if (!createContext())
		{
			Logger::error("GL: Failed to create context");
			cleanup();
			return false;
		}

		m_initialized = true;
		Logger::info("GL: Context initialized successfully");
		return true;
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Exception during initialization: {}", e.what());
		cleanup();
		return false;
	}
}

bool GLContextWGL::createWindow()
{
	try
	{
		if (!m_ownsWindow)
		{
			m_hwnd = reinterpret_cast<HWND>(m_windowInfo.window_handle);
			if (!m_hwnd)
			{
				Logger::error("GL: External window handle is null");
				return false;
			}

			m_hdc = GetDC(m_hwnd);
			if (!m_hdc)
			{
				Logger::error("GL: Failed to get device context for external window: {}", GetLastError());
				return false;
			}

			Logger::info("GL: Using external window for OpenGL context");
			return true;
		}

		WNDCLASSW wc = {};
		wc.lpfnWndProc = WindowProc;
		wc.hInstance = m_hinstance;
		wc.lpszClassName = g_windowClassName;
		wc.style = CS_OWNDC;

		RegisterClassW(&wc);

		m_hwnd = CreateWindowExW(
			0,
			g_windowClassName,
			L"OpenGL Context",
			0,
			0, 0,
			m_width, m_height,
			nullptr,
			nullptr,
			m_hinstance,
			this);

		if (!m_hwnd)
		{
			Logger::error("GL: Failed to create window: {}", GetLastError());
			return false;
		}

		m_hdc = GetDC(m_hwnd);
		if (!m_hdc)
		{
			Logger::error("GL: Failed to get device context: {}", GetLastError());
			DestroyWindow(m_hwnd);
			m_hwnd = nullptr;
			return false;
		}

		Logger::info("GL: Window created successfully");
		return true;
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Exception creating window: {}", e.what());
		return false;
	}
}

bool GLContextWGL::createContext()
{
	try
	{
		if (!m_hdc)
			return false;

		int currentPixelFormat = GetPixelFormat(m_hdc);
		if (currentPixelFormat == 0)
		{
			PIXELFORMATDESCRIPTOR pfd = {};
			pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
			pfd.nVersion = 1;
			pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
			pfd.iPixelType = PFD_TYPE_RGBA;
			pfd.cColorBits = 32;
			pfd.cDepthBits = 24;
			pfd.cStencilBits = 8;
			pfd.iLayerType = PFD_MAIN_PLANE;

			int pixelFormat = ChoosePixelFormat(m_hdc, &pfd);
			if (pixelFormat == 0)
			{
				Logger::error("GL: Failed to choose pixel format: {}", GetLastError());
				return false;
			}

			if (!SetPixelFormat(m_hdc, pixelFormat, &pfd))
			{
				Logger::error("GL: Failed to set pixel format: {}", GetLastError());
				return false;
			}
		}

		m_hglrc = wglCreateContext(m_hdc);
		if (!m_hglrc)
		{
			Logger::error("GL: Failed to create GL context: {}", GetLastError());
			return false;
		}

		if (!wglMakeCurrent(m_hdc, m_hglrc))
		{
			Logger::error("GL: Failed to make context current for VSync setup: {}", GetLastError());
			return false;
		}

		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

		if (wglSwapIntervalEXT)
		{
			wglSwapIntervalEXT(1);
			Logger::info("GL: VSync enabled by default");
		}
		else
		{
			Logger::warn("GL: VSync not supported (wglSwapIntervalEXT not available)");
		}

		wglMakeCurrent(nullptr, nullptr);

		Logger::info("GL: GL context created successfully");
		return true;
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Exception creating context: {}", e.what());
		return false;
	}
}

bool GLContextWGL::makeCurrent()
{
	try
	{
		if (!m_hdc || !m_hglrc)
			return false;

		if (!wglMakeCurrent(m_hdc, m_hglrc))
		{
			Logger::error("GL: Failed to make context current: {}", GetLastError());
			return false;
		}

		return true;
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Exception making context current: {}", e.what());
		return false;
	}
}

void GLContextWGL::releaseCurrent()
{
	try
	{
		wglMakeCurrent(nullptr, nullptr);
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Exception releasing context: {}", e.what());
	}
}

void GLContextWGL::swapBuffers()
{
	try
	{
		if (m_hdc)
		{
			::SwapBuffers(m_hdc);
		}
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Exception swapping buffers: {}", e.what());
	}
}

void GLContextWGL::resize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}

void* GLContextWGL::getProcAddress(const char* name)
{
	void* addr = reinterpret_cast<void*>(wglGetProcAddress(name));
	if (addr)
		return addr;

	static HMODULE opengl32 = nullptr;
	if (!opengl32)
		opengl32 = GetModuleHandleA("opengl32.dll");

	if (opengl32)
		return reinterpret_cast<void*>(GetProcAddress(opengl32, name));

	return nullptr;
}

void GLContextWGL::setVSync(bool enabled)
{
	try
	{
		if (wglSwapIntervalEXT)
		{
			if (wglMakeCurrent(m_hdc, m_hglrc))
			{
				wglSwapIntervalEXT(enabled ? 1 : 0);
				Logger::info("GL: VSync {}", enabled ? "enabled" : "disabled");
				wglMakeCurrent(nullptr, nullptr);
			}
			else
			{
				Logger::error("GL: Failed to make context current for VSync change: {}", GetLastError());
			}
		}
		else
		{
			Logger::warn("GL: Cannot set VSync - wglSwapIntervalEXT not available");
		}
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Exception setting VSync: {}", e.what());
	}
}

void GLContextWGL::cleanup()
{
	try
	{
		releaseCurrent();

		if (m_hglrc)
		{
			wglDeleteContext(m_hglrc);
			m_hglrc = nullptr;
		}

		if (m_hdc && m_hwnd)
		{
			ReleaseDC(m_hwnd, m_hdc);
			m_hdc = nullptr;
		}

		if (m_hwnd && m_ownsWindow)
		{
			DestroyWindow(m_hwnd);
		}
		m_hwnd = nullptr;

		m_initialized = false;
		Logger::info("GL: Cleanup complete");
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Exception during cleanup: {}", e.what());
	}
}

#endif // _WIN32
