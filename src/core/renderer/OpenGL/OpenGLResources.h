// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <cstdint>
#include <cstddef>

typedef unsigned int GLuint;
typedef int GLint;
typedef unsigned int GLenum;

struct OpenGLVertex
{
	int16_t pos[3];
	int16_t normal[3];
	int16_t texcoord[2];
	uint8_t color[4];
};

struct OpenGLBGColor
{
	uint8_t r, g, b, a;
};

class OpenGLResources
{
public:
	OpenGLResources();
	~OpenGLResources();

	OpenGLResources(const OpenGLResources&) = delete;
	OpenGLResources& operator=(const OpenGLResources&) = delete;

	bool createIconBuffers();
	bool createBackgroundBuffers();
	bool createUniformBuffer();
	bool createTexture();
	void destroy();

	void uploadVertexData(const OpenGLVertex* vertices, uint32_t count);
	void uploadTexture(const uint8_t* rgba, uint32_t width, uint32_t height);
	void updateUniformBuffer(const void* data, size_t size);
	void updateBackgroundColors(const OpenGLBGColor* colors, uint32_t count);
	GLuint getIconVAO() const { return m_iconVAO; }
	GLuint getIconVBO() const { return m_iconVBO; }
	GLuint getBackgroundVAO() const { return m_backgroundVAO; }
	GLuint getTextureID() const { return m_textureID; }
	GLuint getUBO() const { return m_UBO; }

private:
	GLuint m_iconVAO = 0;
	GLuint m_iconVBO = 0;
	GLuint m_iconEBO = 0;
	GLuint m_backgroundVAO = 0;
	GLuint m_backgroundVBO = 0;
	GLuint m_backgroundColorVBO = 0;
	GLuint m_textureID = 0;
	GLuint m_UBO = 0;
};
