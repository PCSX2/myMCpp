// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "Logger.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <unordered_map>
#include <cmath>
#include <cstdint>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif
#include <glad/gl.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include "OpenGLRenderer.h"
#include "ps2iconsys.h"
#include "ps2icon.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace fs = std::filesystem;

OpenGLRenderer::OpenGLRenderer(const WindowInfo& windowInfo)
	: m_initialized(false)
	, m_width(windowInfo.surface_width)
	, m_height(windowInfo.surface_height)
	, m_vertexCount(0)
	, m_frameCount(0)
	, m_icon(nullptr)
	, m_animationEnabled(true)
	, m_animStart(std::chrono::steady_clock::now())
	, m_windowInfo(windowInfo)
{
	m_camera.applyMode();
	m_lighting.applyMode();
}

OpenGLRenderer::~OpenGLRenderer()
{
	shutdown();
}

bool OpenGLRenderer::initialize()
{
	try
	{
		std::string error;
		m_context = GLContext::Create(m_windowInfo, &error);
		if (!m_context)
		{
			Logger::error("GL: Failed to create GL context: {}", error);
			return false;
		}

		if (!m_context->makeCurrent())
		{
			Logger::error("GL: Failed to make context current");
			m_context.reset();
			return false;
		}

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_FRAMEBUFFER_SRGB);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		if (!m_shader.loadIconShaders())
		{
			Logger::error("GL: Failed to setup icon shaders");
			m_context->releaseCurrent();
			m_context.reset();
			return false;
		}

		if (!m_shader.loadBackgroundShaders())
		{
			Logger::error("GL: Failed to setup background shaders");
			m_context->releaseCurrent();
			m_context.reset();
			return false;
		}

		if (!m_resources.createIconBuffers())
		{
			Logger::error("GL: Failed to create icon buffers");
			m_context->releaseCurrent();
			m_context.reset();
			return false;
		}

		if (!m_resources.createBackgroundBuffers())
		{
			Logger::error("GL: Failed to create background buffers");
			m_context->releaseCurrent();
			m_context.reset();
			return false;
		}

		if (!m_resources.createUniformBuffer())
		{
			Logger::error("GL: Failed to create uniform buffer");
			m_context->releaseCurrent();
			m_context.reset();
			return false;
		}

		if (!m_resources.createTexture())
		{
			Logger::error("GL: Failed to create texture");
			m_context->releaseCurrent();
			m_context.reset();
			return false;
		}

		m_context->releaseCurrent();

		m_initialized = true;
		Logger::info("GL: Renderer initialized successfully");
		return true;
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Initialization failed: {}", e.what());
		if (m_context)
		{
			m_context->releaseCurrent();
			m_context.reset();
		}
		return false;
	}
}

void OpenGLRenderer::shutdown()
{
	if (!m_initialized)
		return;

	try
	{
		if (m_context)
		{
			m_context->makeCurrent();
		}

		m_resources.destroy();
		m_shader.destroy();

		if (m_context)
		{
			m_context->releaseCurrent();
			m_context.reset();
		}

		m_initialized = false;
		Logger::info("GL: Shutdown complete");
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Error during shutdown: {}", e.what());
	}
}

void OpenGLRenderer::setIcon(std::shared_ptr<PS2Icon::Icon> icon)
{
	try
	{
		m_icon = icon;
		if (m_icon)
		{
			m_animStart = std::chrono::steady_clock::now();
			prepareVertexData();
			setupTexture();
			Logger::info("GL: Icon loaded successfully");
		}
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Error setting icon: {}", e.what());
	}
}

void OpenGLRenderer::prepareVertexData()
{
	if (!m_icon || !m_context)
		return;

	try
	{
		m_context->makeCurrent();

		m_vertexCount = m_icon->getVertexCount();

		if (m_vertexCount == 0)
		{
			Logger::error("GL: No vertices in icon");
			m_context->releaseCurrent();
			return;
		}

		const auto* vertexData = m_icon->getVertexData(0);
		const auto* normalData = m_icon->getNormalUVData();
		const auto* colorData = m_icon->getColorData();

		if (!vertexData || !normalData || !colorData)
		{
			Logger::error("GL: Missing vertex data components");
			m_context->releaseCurrent();
			return;
		}

		std::vector<OpenGLVertex> vertices(m_vertexCount);

		for (uint32_t i = 0; i < m_vertexCount; ++i)
		{
			vertices[i].pos[0] = vertexData[i].x;
			vertices[i].pos[1] = vertexData[i].y;
			vertices[i].pos[2] = vertexData[i].z;
			vertices[i].normal[0] = normalData[i].nx;
			vertices[i].normal[1] = normalData[i].ny;
			vertices[i].normal[2] = normalData[i].nz;
			vertices[i].texcoord[0] = normalData[i].u;
			vertices[i].texcoord[1] = normalData[i].v;
			vertices[i].color[0] = colorData[i].r;
			vertices[i].color[1] = colorData[i].g;
			vertices[i].color[2] = colorData[i].b;
			vertices[i].color[3] = colorData[i].a;
		}

		m_resources.uploadVertexData(vertices.data(), m_vertexCount);

		m_context->releaseCurrent();
		Logger::info("GL: Prepared {} vertices", m_vertexCount);
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Error preparing vertex data: {}", e.what());
		if (m_context)
			m_context->releaseCurrent();
	}
}

void OpenGLRenderer::setupTexture()
{
	if (!m_icon || !m_context)
		return;

	try
	{
		m_context->makeCurrent();

		const auto* textureData = m_icon->getTextureData();
		if (!textureData)
		{
			Logger::error("GL: No texture data in icon");
			m_context->releaseCurrent();
			return;
		}

		uint32_t texWidth = m_icon->getTextureWidth();
		uint32_t texHeight = m_icon->getTextureHeight();

		if (texWidth == 0 || texHeight == 0)
		{
			Logger::error("GL: Invalid texture dimensions");
			m_context->releaseCurrent();
			return;
		}

		auto rgba = TextureUtils::convertPS2TextureToRGBA(textureData, texWidth, texHeight);

		m_resources.uploadTexture(rgba.data(), texWidth, texHeight);

		m_context->releaseCurrent();
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Error setting up texture: {}", e.what());
		if (m_context)
			m_context->releaseCurrent();
	}
}

void OpenGLRenderer::setRotation(float x, float y, float z)
{
	m_camera.setRotation(x, y, z);
}

void OpenGLRenderer::setZoom(float zoom)
{
	m_camera.setZoom(zoom);
}

void OpenGLRenderer::setCameraMode(CameraMode mode)
{
	m_camera.mode = mode;
	m_camera.applyMode();
}

void OpenGLRenderer::setLightingFromIconSys(PS2IconSys* iconSys)
{
	m_lighting.loadFromIconSys(iconSys);
}

void OpenGLRenderer::setLightingMode(LightingMode mode)
{
	m_lighting.mode = mode;
	m_lighting.applyMode();
}

void OpenGLRenderer::setBackgroundFromIconSys(PS2IconSys* iconSys)
{
	if (m_background.loadFromIconSys(iconSys))
	{
		updateBackgroundVertexData();
	}
}

void OpenGLRenderer::updateBackgroundVertexData()
{
	if (!m_context)
		return;

	m_context->makeCurrent();

	OpenGLBGColor bgColorData[4];
	const auto& colors = m_background.colors;
	bgColorData[0] = {static_cast<uint8_t>(colors[0].x * 255.0f), static_cast<uint8_t>(colors[0].y * 255.0f), static_cast<uint8_t>(colors[0].z * 255.0f), static_cast<uint8_t>(colors[0].w * 255.0f)}; // top-left
	bgColorData[1] = {static_cast<uint8_t>(colors[2].x * 255.0f), static_cast<uint8_t>(colors[2].y * 255.0f), static_cast<uint8_t>(colors[2].z * 255.0f), static_cast<uint8_t>(colors[2].w * 255.0f)}; // bottom-left
	bgColorData[2] = {static_cast<uint8_t>(colors[1].x * 255.0f), static_cast<uint8_t>(colors[1].y * 255.0f), static_cast<uint8_t>(colors[1].z * 255.0f), static_cast<uint8_t>(colors[1].w * 255.0f)}; // top-right
	bgColorData[3] = {static_cast<uint8_t>(colors[3].x * 255.0f), static_cast<uint8_t>(colors[3].y * 255.0f), static_cast<uint8_t>(colors[3].z * 255.0f), static_cast<uint8_t>(colors[3].w * 255.0f)}; // bottom-right

	m_resources.updateBackgroundColors(bgColorData, 4);
	m_context->releaseCurrent();
}

void OpenGLRenderer::setBackgroundColor(float r, float g, float b, float a)
{
	m_background.setColor(r, g, b, a);
	updateBackgroundVertexData();
}

void OpenGLRenderer::render()
{
	if (!m_initialized || !m_icon || m_vertexCount == 0 || m_shader.getIconProgram() == 0 || !m_context)
		return;

	try
	{
		if (!m_context->makeCurrent())
		{
			Logger::error("GL: Failed to make context current for rendering");
			return;
		}

		glViewport(0, 0, m_width, m_height);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (m_background.shouldRender && m_shader.getBackgroundProgram() != 0 && m_resources.getBackgroundVAO() != 0)
		{
			glUseProgram(m_shader.getBackgroundProgram());
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBindVertexArray(m_resources.getBackgroundVAO());
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glBindVertexArray(0);
			glDepthMask(GL_TRUE);
			glEnable(GL_DEPTH_TEST);
		}

		glUseProgram(m_shader.getIconProgram());

		if (m_animationEnabled && m_icon->getAnimationShapes() > 1 && m_icon->getFrameCount() > 0)
		{
			double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_animStart).count();
			float duration = static_cast<float>(m_icon->getFrameLength());
			float animTime = AnimationUtils::computeAnimationTime(elapsed, duration);

			auto shapeWeights = AnimationUtils::computeShapeWeights(m_icon.get(), animTime, duration);
			auto blendedVerts = AnimationUtils::blendVertices(m_icon.get(), shapeWeights, m_vertexCount);

			std::vector<OpenGLVertex> vertices(m_vertexCount);
			for (uint32_t i = 0; i < m_vertexCount; ++i)
			{
				vertices[i].pos[0] = static_cast<int16_t>(blendedVerts[i].pos[0]);
				vertices[i].pos[1] = static_cast<int16_t>(blendedVerts[i].pos[1]);
				vertices[i].pos[2] = static_cast<int16_t>(blendedVerts[i].pos[2]);
				vertices[i].normal[0] = static_cast<int16_t>(blendedVerts[i].normal[0]);
				vertices[i].normal[1] = static_cast<int16_t>(blendedVerts[i].normal[1]);
				vertices[i].normal[2] = static_cast<int16_t>(blendedVerts[i].normal[2]);
				vertices[i].texcoord[0] = static_cast<int16_t>(blendedVerts[i].texCoord[0]);
				vertices[i].texcoord[1] = static_cast<int16_t>(blendedVerts[i].texCoord[1]);
				vertices[i].color[0] = blendedVerts[i].color[0];
				vertices[i].color[1] = blendedVerts[i].color[1];
				vertices[i].color[2] = blendedVerts[i].color[2];
				vertices[i].color[3] = blendedVerts[i].color[3];
			}

			glBindBuffer(GL_ARRAY_BUFFER, m_resources.getIconVBO());
			glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(OpenGLVertex), vertices.data());
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		float autoRotateY = 0.0f;
		if (m_animationEnabled)
		{
			double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_animStart).count();
			autoRotateY = static_cast<float>(elapsed * -0.5); // ~0.5 radians/sec rotation speed (inverted)
		}
		auto matrices = CameraUtils::computeMatrices(m_camera, m_width, m_height, autoRotateY);
		auto uboData = UniformBufferUtils::buildUniformBufferData(matrices, m_lighting, false);

		m_resources.updateUniformBuffer(&uboData, sizeof(uboData));

		GLuint uboBlockIndex = m_shader.getUniformBlockIndex();
		glUniformBlockBinding(m_shader.getIconProgram(), uboBlockIndex, 0);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_resources.getUBO());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_resources.getTextureID());
		glUniform1i(m_shader.getTexSamplerLocation(), 0);

		glUniform1i(m_shader.getUseTextureLocation(), 1);

		int hasAlpha = (m_icon && m_icon->hasAlpha()) ? 1 : 0;
		glUniform1i(m_shader.getEnableAlphaLocation(), hasAlpha);

		glUniform1f(m_shader.getAlphaOverrideLocation(), 1.0f);

		glBindVertexArray(m_resources.getIconVAO());
		glDisable(GL_CULL_FACE);
		glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
		glBindVertexArray(0);

		m_context->swapBuffers();

		m_context->releaseCurrent();

		m_frameCount++;
	}
	catch (const std::exception& e)
	{
		Logger::error("GL: Error during rendering: {}", e.what());
	}
}

void OpenGLRenderer::resize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}
