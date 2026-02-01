// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "../Renderer.h"
#include "WindowInfo.h"
#include "../Common/RendererCommon.h"
#include "GLContext.h"
#include "OpenGLShader.h"
#include "OpenGLResources.h"
#include <cstdint>
#include <memory>
#include <chrono>

namespace PS2Icon
{
	class Icon;
}

class OpenGLRenderer : public Renderer
{
public:
	explicit OpenGLRenderer(const WindowInfo& windowInfo);
	~OpenGLRenderer();

	bool initialize() override;
	void shutdown() override;
	bool isInitialized() const override { return m_initialized; }

	void setIcon(std::shared_ptr<PS2Icon::Icon> icon) override;
	bool hasValidIcon() const override { return m_icon != nullptr; }

	void setAnimationEnabled(bool enabled) override;
	bool isAnimationEnabled() const override { return m_animationEnabled; }

	void setRotation(float x, float y, float z) override;
	void setZoom(float zoom) override;
	void setCameraOffset(float x, float y, float z) override;
	void resetCamera() override;
	void setCameraMode(CameraMode mode) override;

	void setLightingFromIconSys(PS2IconSys* iconSys) override;
	void setLightingMode(LightingMode mode) override;
	void setBackgroundFromIconSys(PS2IconSys* iconSys) override;
	void setBackgroundColor(float r, float g, float b, float a = 1.0f) override;
	void render() override;
	void resize(uint32_t width, uint32_t height) override;

	uint32_t getVertexCount() const override { return m_vertexCount; }
	uint32_t getFrameCount() const override { return m_frameCount; }

private:
	void prepareVertexData();
	void setupTexture();
	void updateBackgroundVertexData();

	bool m_initialized;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_vertexCount;
	uint32_t m_frameCount;

	std::shared_ptr<PS2Icon::Icon> m_icon;
	bool m_animationEnabled;
	std::chrono::steady_clock::time_point m_animStart;

	LightingState m_lighting;
	CameraState m_camera;
	BackgroundState m_background;
	OpenGLShader m_shader;
	OpenGLResources m_resources;
	std::unique_ptr<GLContext> m_context;
	WindowInfo m_windowInfo;
};
