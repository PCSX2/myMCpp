// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#if defined(__linux__)

#include "GLContext.h"

#include <EGL/egl.h>
#include <cstdint>
#include <memory>
#include <string>

class GLContextEGL : public GLContext
{
public:
	static std::unique_ptr<GLContext> Create(const WindowInfo& windowInfo, Error* error = nullptr);

	~GLContextEGL() override;

	bool initialize() override;
	bool makeCurrent() override;
	void releaseCurrent() override;
	void swapBuffers() override;
	bool isValid() const override { return m_context != EGL_NO_CONTEXT; }
	void resize(uint32_t width, uint32_t height) override;
	void* getProcAddress(const char* name) override;
	void setVSync(bool enabled) override;

protected:
	GLContextEGL(const WindowInfo& windowInfo);

	EGLDisplay m_display;
	EGLContext m_context;
	EGLSurface m_surface;
	EGLConfig m_config;
	struct wl_egl_window* m_waylandWindow = nullptr;

	bool initializeDisplay();
	bool chooseConfig();
	bool createSurface();
	bool createContext();
	void cleanup();

	bool m_initialized;
};

#endif // __linux__
