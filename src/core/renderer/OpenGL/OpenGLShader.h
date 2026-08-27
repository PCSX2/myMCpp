// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <cstdint>
#include <string>
#include "common/Error.h"

typedef unsigned int GLuint;
typedef int GLint;
typedef unsigned int GLenum;

class OpenGLShader
{
public:
	OpenGLShader();
	~OpenGLShader();

	OpenGLShader(const OpenGLShader&) = delete;
	OpenGLShader& operator=(const OpenGLShader&) = delete;

	bool loadIconShaders();
	bool loadBackgroundShaders();
	void destroy();

	const Error& GetError() const { return m_error; }

	GLuint getIconProgram() const { return m_iconProgram; }
	GLuint getBackgroundProgram() const { return m_backgroundProgram; }

	GLint getUniformBlockIndex() const { return m_uboBlockIndex; }
	GLint getTexSamplerLocation() const { return m_texSamplerLocation; }
	GLint getUseTextureLocation() const { return m_useTextureLocation; }
	GLint getEnableAlphaLocation() const { return m_enableAlphaLocation; }
	GLint getAlphaOverrideLocation() const { return m_alphaOverrideLocation; }

private:
	std::string readShaderFile(const std::string& filename);
	GLuint compileShader(const std::string& source, GLenum shaderType);
	GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader);

	Error m_error;
	GLuint m_iconProgram = 0;
	GLuint m_backgroundProgram = 0;

	GLint m_uboBlockIndex = -1;
	GLint m_texSamplerLocation = -1;
	GLint m_useTextureLocation = -1;
	GLint m_enableAlphaLocation = -1;
	GLint m_alphaOverrideLocation = -1;
};
