// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GLContext.h"

#if defined(__APPLE__)
#if defined(__OBJC__)
@class NSOpenGLContext;
@class NSOpenGLView;
@class NSView;
#else
typedef void NSOpenGLContext;
typedef void NSOpenGLView;
typedef void NSView;
#endif
#endif

class GLContextAGL : public GLContext
{
public:
	~GLContextAGL() override;

	static std::unique_ptr<GLContext> Create(const WindowInfo& windowInfo, std::string* error);

	bool initialize() override;
	bool makeCurrent() override;
	void releaseCurrent() override;
	void swapBuffers() override;
	bool isValid() const override;
	void resize(uint32_t width, uint32_t height) override;
	void* getProcAddress(const char* name) override;
	void setVSync(bool enabled) override;

private:
	GLContextAGL(const WindowInfo& windowInfo);

	NSOpenGLContext* m_context = nullptr;
	NSOpenGLView* m_glView = nullptr;
	NSView* m_view = nullptr;
};
