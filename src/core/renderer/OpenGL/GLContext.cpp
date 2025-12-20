// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+
// Based on code from PCSX2: (https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS/Renderers/OpenGL/GLContext.cpp)

#include "GLContext.h"

#if defined(_WIN32)
#include "GLContextWGL.h"
#elif defined(__APPLE__)
#include "GLContextAGL.h"
#elif defined(__linux__)
#include "GLContextEGL.h"
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif
#include <glad/gl.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include "../../../common/Logger.h"

GLContext::GLContext(const WindowInfo& windowInfo)
	: m_windowInfo(windowInfo)
	, m_width(windowInfo.surface_width)
	, m_height(windowInfo.surface_height)
{
}

GLContext::~GLContext() = default;

std::unique_ptr<GLContext> GLContext::Create(const WindowInfo& windowInfo, std::string* error)
{
	std::unique_ptr<GLContext> context;

#if defined(_WIN32)
	context = GLContextWGL::Create(windowInfo, error);
#elif defined(__APPLE__)
	context = GLContextAGL::Create(windowInfo, error);
#elif defined(__linux__)
	context = GLContextEGL::Create(windowInfo, error);
#else
	if (error)
		*error = "Unsupported platform for OpenGL context creation";
	return nullptr;
#endif

	if (!context)
		return nullptr;

	if (!context->initialize())
	{
		if (error)
			*error = "Failed to initialize OpenGL context";
		return nullptr;
	}

	if (!context->makeCurrent())
	{
		if (error)
			*error = "Failed to make OpenGL context current";
		return nullptr;
	}

	// NOTE: Not thread-safe, but we're not creating more than one context at a time.
	static GLContext* contextBeingCreated = nullptr;
	contextBeingCreated = context.get();

	if (!gladLoadGL([](const char* name) {
			return reinterpret_cast<GLADapiproc>(contextBeingCreated->getProcAddress(name));
		}))
	{
		context->releaseCurrent();
		if (error)
			*error = "Failed to load GL functions for GLAD";
		return nullptr;
	}

	contextBeingCreated = nullptr;

	context->releaseCurrent();

	Logger::info("GLContext: Created successfully with OpenGL {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

	return context;
}
