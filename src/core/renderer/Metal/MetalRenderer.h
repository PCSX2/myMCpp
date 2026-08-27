// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "core/renderer/Renderer.h"
#include "common/WindowInfo.h"
#include "core/renderer/Common/RendererCommon.h"

#include <vector>
#include <memory>
#include <chrono>

class Config;

namespace PS2Icon
{
	class Icon;
}

struct MetalRendererImpl;

class MetalRenderer : public Renderer
{
public:
	explicit MetalRenderer(const WindowInfo& windowInfo, Config* config = nullptr);
	~MetalRenderer();

	bool initialize() override;
	void shutdown() override;
	bool isInitialized() const override { return m_initialized; }

	void setVSync(bool enabled) override;

	void setIcon(std::shared_ptr<PS2Icon::Icon> icon) override;
	bool hasValidIcon() const override;

	void setAnimationEnabled(bool enabled) override { m_animationEnabled = enabled; }
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
	void renderFrame();

	bool m_initialized = false;
	WindowInfo m_windowInfo;
	uint32_t m_vertexCount = 0;
	uint32_t m_frameCount = 0;
	uint32_t m_frameIndex = 0;

	std::shared_ptr<PS2Icon::Icon> m_icon;
	bool m_animationEnabled = true;

	bool m_iconChanged = false;

	std::chrono::steady_clock::time_point m_animStart;

	LightingState m_lighting;
	CameraState m_camera;
	BackgroundState m_background;
	Config* m_config;

	std::unique_ptr<MetalRendererImpl> m_impl;
};
