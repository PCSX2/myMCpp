// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "GLContextAGL.h"

#if defined(__APPLE__)

#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>
#include <dlfcn.h>

#include "Logger.h"

GLContextAGL::GLContextAGL(const WindowInfo& windowInfo)
	: GLContext(windowInfo)
{
}

GLContextAGL::~GLContextAGL()
{
	if (m_context)
	{
		[NSOpenGLContext clearCurrentContext];
		m_context = nil;
	}
	if (m_pixelFormat)
	{
		m_pixelFormat = nil;
	}
}

std::unique_ptr<GLContext> GLContextAGL::Create(const WindowInfo& windowInfo, [[maybe_unused]] std::string* error)
{
	auto context = std::unique_ptr<GLContextAGL>(new GLContextAGL(windowInfo));
	
	context->m_view = (__bridge NSView*)windowInfo.window_handle;
	
	return context;
}

bool GLContextAGL::initialize()
{
	NSOpenGLPixelFormatAttribute attrs[] = {
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFANoRecovery,
		0
	};
	
	m_pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
	if (!m_pixelFormat)
	{
		Logger::error("GLContextAGL: Failed to create pixel format");
		return false;
	}
	
	m_context = [[NSOpenGLContext alloc] initWithFormat:m_pixelFormat shareContext:nil];
	if (!m_context)
	{
		Logger::error("GLContextAGL: Failed to create OpenGL context");
		return false;
	}
	
	if (m_view)
	{
		[m_context setView:m_view];
	}
	
	GLint swapInterval = 1;
	[m_context setValues:&swapInterval forParameter:NSOpenGLContextParameterSwapInterval];
	
	Logger::info("GLContextAGL: Created OpenGL 3.2 Core context");
	
	return true;
}

bool GLContextAGL::makeCurrent()
{
	if (!m_context)
		return false;
	
	[m_context makeCurrentContext];
	return true;
}

void GLContextAGL::releaseCurrent()
{
	[NSOpenGLContext clearCurrentContext];
}

void GLContextAGL::swapBuffers()
{
	if (m_context)
	{
		[m_context flushBuffer];
	}
}

bool GLContextAGL::isValid() const
{
	return m_context != nullptr;
}

void GLContextAGL::resize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
	
	if (m_context)
	{
		[m_context update];
	}
}

void* GLContextAGL::getProcAddress(const char* name)
{
	// On macOS, we use dlsym to get OpenGL function pointers
	static void* openglFramework = nullptr;
	if (!openglFramework)
	{
		openglFramework = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY);
	}
	
	if (openglFramework)
	{
		return dlsym(openglFramework, name);
	}
	
	return nullptr;
}

void GLContextAGL::setVSync(bool enabled)
{
	if (m_context)
	{
		GLint swapInterval = enabled ? 1 : 0;
		[m_context setValues:&swapInterval forParameter:NSOpenGLContextParameterSwapInterval];
	}
}

#endif // __APPLE__
