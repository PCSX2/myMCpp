// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#ifdef _WIN32

#include "GLContext.h"

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
#include <cstdint>
#include <memory>
#include <string>

class GLContextWGL : public GLContext
{
public:
	static std::unique_ptr<GLContext> Create(const WindowInfo& windowInfo, std::string* error = nullptr);

	~GLContextWGL() override;

	bool initialize() override;
	bool makeCurrent() override;
	void releaseCurrent() override;
	void swapBuffers() override;
	bool isValid() const override { return m_hglrc != nullptr; }
	void resize(uint32_t width, uint32_t height) override;
	void* getProcAddress(const char* name) override;
	void setVSync(bool enabled) override;

private:
	GLContextWGL(const WindowInfo& windowInfo);

	bool createWindow();
	bool createContext();
	void cleanup();

	HWND m_hwnd;
	HDC m_hdc;
	HGLRC m_hglrc;
	HINSTANCE m_hinstance;
	bool m_ownsWindow;

	bool m_initialized;
};

#endif // _WIN32
