// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "WindowInfo.h"
#include <cstdint>
#include <memory>
#include <string>

class Error;

class GLContext
{
public:
	struct Version
	{
		int major;
		int minor;
	};

	virtual ~GLContext();

	static std::unique_ptr<GLContext> Create(const WindowInfo& windowInfo, Error* error = nullptr);

	virtual bool initialize() = 0;
	virtual bool makeCurrent() = 0;
	virtual void releaseCurrent() = 0;
	virtual void swapBuffers() = 0;
	virtual bool isValid() const = 0;
	virtual void resize(uint32_t width, uint32_t height) = 0;
	virtual void* getProcAddress(const char* name) = 0;
	virtual void setVSync(bool enabled) = 0;

	uint32_t getWidth() const { return m_width; }
	uint32_t getHeight() const { return m_height; }

	void setCreationError(Error* error) { m_creationError = error; }

protected:
	GLContext(const WindowInfo& windowInfo);

	bool failCreation(std::string message);

	WindowInfo m_windowInfo;
	uint32_t m_width;
	uint32_t m_height;
	Error* m_creationError = nullptr;
};
