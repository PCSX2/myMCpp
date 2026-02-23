// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "OpenGLShader.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif
#include <glad/gl.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <fstream>
#include <sstream>
#include "Logger.h"
#include "ResourcePath.h"
#include <filesystem>

namespace fs = std::filesystem;

OpenGLShader::OpenGLShader() = default;

OpenGLShader::~OpenGLShader()
{
	destroy();
}

void OpenGLShader::destroy()
{
	if (m_iconProgram != 0)
	{
		glDeleteProgram(m_iconProgram);
		m_iconProgram = 0;
	}
	if (m_backgroundProgram != 0)
	{
		glDeleteProgram(m_backgroundProgram);
		m_backgroundProgram = 0;
	}
}

std::string OpenGLShader::readShaderFile(const std::string& filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		Logger::error("GL: Failed to open shader file: {}", filename);
		return "";
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

GLuint OpenGLShader::compileShader(const std::string& source, GLenum shaderType)
{
	GLuint shader = glCreateShader(shaderType);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		Logger::error("GL: Shader compilation failed:\n{}", infoLog);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint OpenGLShader::linkProgram(GLuint vertexShader, GLuint fragmentShader)
{
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	int success;
	char infoLog[512];
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		Logger::error("GL: Program linking failed:\n{}", infoLog);
		glDeleteProgram(program);
		return 0;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return program;
}

bool OpenGLShader::loadIconShaders()
{
	fs::path vertexShaderPath = ResourcePath::shaders() / "OpenGL" / "icon.vert";
	fs::path fragmentShaderPath = ResourcePath::shaders() / "OpenGL" / "icon.frag";

	std::string vertSource = readShaderFile(vertexShaderPath.string());
	std::string fragSource = readShaderFile(fragmentShaderPath.string());

	if (vertSource.empty() || fragSource.empty())
	{
		Logger::error("GL: Failed to load icon shader sources");
		return false;
	}

	GLuint vertexShader = compileShader(vertSource, GL_VERTEX_SHADER);
	if (vertexShader == 0)
	{
		Logger::error("GL: Failed to compile icon vertex shader");
		return false;
	}

	GLuint fragmentShader = compileShader(fragSource, GL_FRAGMENT_SHADER);
	if (fragmentShader == 0)
	{
		glDeleteShader(vertexShader);
		Logger::error("GL: Failed to compile icon fragment shader");
		return false;
	}

	m_iconProgram = linkProgram(vertexShader, fragmentShader);
	if (m_iconProgram == 0)
	{
		Logger::error("GL: Failed to link icon shader program");
		return false;
	}

	m_uboBlockIndex = glGetUniformBlockIndex(m_iconProgram, "UniformBufferObject");
	m_texSamplerLocation = glGetUniformLocation(m_iconProgram, "texSampler");
	m_useTextureLocation = glGetUniformLocation(m_iconProgram, "useTexture");
	m_enableAlphaLocation = glGetUniformLocation(m_iconProgram, "enableAlpha");
	m_alphaOverrideLocation = glGetUniformLocation(m_iconProgram, "alphaOverride");

	Logger::info("GL: Icon shaders loaded and compiled successfully");
	return true;
}

bool OpenGLShader::loadBackgroundShaders()
{
	fs::path bgVertexShaderPath = ResourcePath::shaders() / "OpenGL" / "background.vert";
	fs::path bgFragmentShaderPath = ResourcePath::shaders() / "OpenGL" / "background.frag";

	std::string bgVertSource = readShaderFile(bgVertexShaderPath.string());
	std::string bgFragSource = readShaderFile(bgFragmentShaderPath.string());

	if (bgVertSource.empty() || bgFragSource.empty())
	{
		Logger::error("GL: Failed to load background shader sources");
		return false;
	}

	GLuint bgVertexShader = compileShader(bgVertSource, GL_VERTEX_SHADER);
	if (bgVertexShader == 0)
	{
		Logger::error("GL: Failed to compile background vertex shader");
		return false;
	}

	GLuint bgFragmentShader = compileShader(bgFragSource, GL_FRAGMENT_SHADER);
	if (bgFragmentShader == 0)
	{
		glDeleteShader(bgVertexShader);
		Logger::error("GL: Failed to compile background fragment shader");
		return false;
	}

	m_backgroundProgram = linkProgram(bgVertexShader, bgFragmentShader);
	if (m_backgroundProgram == 0)
	{
		Logger::error("GL: Failed to link background shader program");
		return false;
	}

	Logger::info("GL: Background shaders loaded and compiled successfully");
	return true;
}
