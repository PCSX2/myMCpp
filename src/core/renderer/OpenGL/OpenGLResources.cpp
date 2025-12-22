// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "OpenGLResources.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif
#include <glad/gl.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include "Logger.h"
#include <cstring>

OpenGLResources::OpenGLResources() = default;

OpenGLResources::~OpenGLResources()
{
	destroy();
}

void OpenGLResources::destroy()
{
	if (m_textureID != 0)
	{
		glDeleteTextures(1, &m_textureID);
		m_textureID = 0;
	}

	if (m_UBO != 0)
	{
		glDeleteBuffers(1, &m_UBO);
		m_UBO = 0;
	}

	if (m_iconEBO != 0)
	{
		glDeleteBuffers(1, &m_iconEBO);
		m_iconEBO = 0;
	}

	if (m_iconVBO != 0)
	{
		glDeleteBuffers(1, &m_iconVBO);
		m_iconVBO = 0;
	}

	if (m_iconVAO != 0)
	{
		glDeleteVertexArrays(1, &m_iconVAO);
		m_iconVAO = 0;
	}

	if (m_backgroundColorVBO != 0)
	{
		glDeleteBuffers(1, &m_backgroundColorVBO);
		m_backgroundColorVBO = 0;
	}

	if (m_backgroundVBO != 0)
	{
		glDeleteBuffers(1, &m_backgroundVBO);
		m_backgroundVBO = 0;
	}

	if (m_backgroundVAO != 0)
	{
		glDeleteVertexArrays(1, &m_backgroundVAO);
		m_backgroundVAO = 0;
	}
}

bool OpenGLResources::createIconBuffers()
{
	glGenVertexArrays(1, &m_iconVAO);
	glGenBuffers(1, &m_iconVBO);
	glGenBuffers(1, &m_iconEBO);

	if (m_iconVAO == 0 || m_iconVBO == 0)
	{
		Logger::error("GL: Failed to create icon buffers");
		return false;
	}

	return true;
}

bool OpenGLResources::createBackgroundBuffers()
{
	float quadVertices[] = {
		-1.0f, 1.0f, // top-left
		-1.0f, -1.0f, // bottom-left
		1.0f, 1.0f, // top-right
		1.0f, -1.0f // bottom-right
	};

	glGenVertexArrays(1, &m_backgroundVAO);
	glGenBuffers(1, &m_backgroundVBO);
	glGenBuffers(1, &m_backgroundColorVBO);

	if (m_backgroundVAO == 0 || m_backgroundVBO == 0 || m_backgroundColorVBO == 0)
	{
		Logger::error("GL: Failed to create background buffers");
		return false;
	}

	glBindVertexArray(m_backgroundVAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_backgroundVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	OpenGLBGColor bgColorData[4] = {
		{255, 255, 255, 255},
		{255, 255, 255, 255},
		{255, 255, 255, 255},
		{255, 255, 255, 255}};

	glBindBuffer(GL_ARRAY_BUFFER, m_backgroundColorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bgColorData), bgColorData, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(OpenGLBGColor), (void*)0);
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return true;
}

bool OpenGLResources::createUniformBuffer()
{
	glGenBuffers(1, &m_UBO);
	if (m_UBO == 0)
	{
		Logger::error("GL: Failed to create uniform buffer");
		return false;
	}

	glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
	glBufferData(GL_UNIFORM_BUFFER, 512, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return true;
}

bool OpenGLResources::createTexture()
{
	glGenTextures(1, &m_textureID);
	if (m_textureID == 0)
	{
		Logger::error("GL: Failed to create texture");
		return false;
	}

	return true;
}

void OpenGLResources::uploadVertexData(const OpenGLVertex* vertices, uint32_t count)
{
	if (!vertices || count == 0 || m_iconVAO == 0)
		return;

	glBindVertexArray(m_iconVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_iconVBO);
	glBufferData(GL_ARRAY_BUFFER, count * sizeof(OpenGLVertex), vertices, GL_DYNAMIC_DRAW);

	const size_t stride = sizeof(OpenGLVertex);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_SHORT, GL_FALSE, stride, (void*)offsetof(OpenGLVertex, pos));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_SHORT, GL_FALSE, stride, (void*)offsetof(OpenGLVertex, normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_SHORT, GL_FALSE, stride, (void*)offsetof(OpenGLVertex, texcoord));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offsetof(OpenGLVertex, color));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void OpenGLResources::uploadTexture(const uint8_t* rgba, uint32_t width, uint32_t height)
{
	if (!rgba || width == 0 || height == 0 || m_textureID == 0)
		return;

	glBindTexture(GL_TEXTURE_2D, m_textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_SRGB8_ALPHA8,
		width,
		height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		rgba);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	Logger::info("GL: Texture loaded: {}x{}", width, height);
}

void OpenGLResources::updateUniformBuffer(const void* data, size_t size)
{
	if (!data || size == 0 || m_UBO == 0)
		return;

	glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OpenGLResources::updateBackgroundColors(const OpenGLBGColor* colors, uint32_t count)
{
	if (!colors || count == 0 || m_backgroundColorVBO == 0)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, m_backgroundColorVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(OpenGLBGColor), colors);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
