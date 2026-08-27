// SPDX-FileCopyrightText: 2025-2026 SternXD
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
#include "common/Error.h"
#include "common/Logger.h"

GLContext::GLContext(const WindowInfo& windowInfo)
	: m_windowInfo(windowInfo)
	, m_width(windowInfo.surface_width)
	, m_height(windowInfo.surface_height)
{
}

GLContext::~GLContext() = default;

bool GLContext::failCreation(std::string message)
{
	if (m_creationError)
		return Error::Fail(m_creationError, std::move(message));
	Logger::error("{}", message);
	return false;
}

std::unique_ptr<GLContext> GLContext::Create(const WindowInfo& windowInfo, Error* error)
{
	std::unique_ptr<GLContext> context;

#if defined(_WIN32)
	context = GLContextWGL::Create(windowInfo, error);
#elif defined(__APPLE__)
	context = GLContextAGL::Create(windowInfo, error);
#elif defined(__linux__)
	context = GLContextEGL::Create(windowInfo, error);
#else
	Error::Fail(error, "GL: Unsupported platform for OpenGL context creation");
	return nullptr;
#endif

	if (!context)
		return nullptr;

	context->setCreationError(error);

	if (!context->initialize())
	{
		if (!error || !error->IsValid())
			Error::Fail(error, "GL: Failed to initialize OpenGL context");
		return nullptr;
	}

	if (!context->makeCurrent())
	{
		if (!error || !error->IsValid())
			Error::Fail(error, "GL: Failed to make OpenGL context current");
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
		Error::Fail(error, "GL: Failed to load GL functions for GLAD");
		return nullptr;
	}

	contextBeingCreated = nullptr;

	const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	Logger::info("GL: Using device: {} (OpenGL {})",
		renderer ? renderer : "unknown",
		version ? version : "unknown");

	context->releaseCurrent();
	context->setCreationError(nullptr);

	return context;
}
