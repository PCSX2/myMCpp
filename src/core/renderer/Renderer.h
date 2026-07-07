// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include "../../common/Error.h"
#include "WindowInfo.h"

class Config;

namespace PS2Icon
{
	class Icon;
}

enum class LightingMode
{
	Off,
	Icon,
	Alternate1,
	Alternate2
};

enum class CameraMode
{
	Default,
	Flat,
	Near,
	High
};

class PS2IconSys;

class Renderer
{
public:
	virtual ~Renderer() = default;

	const Error& GetError() const { return m_error; }

	virtual bool initialize() = 0;
	virtual void shutdown() = 0;
	virtual bool isInitialized() const = 0;

	virtual void setIcon(std::shared_ptr<PS2Icon::Icon> icon) = 0;
	virtual bool hasValidIcon() const = 0;

	virtual void setAnimationEnabled(bool enabled) = 0;
	virtual bool isAnimationEnabled() const = 0;

	virtual void setRotation(float x, float y, float z) = 0;
	virtual void setZoom(float zoom) = 0;
	virtual void setCameraOffset(float x, float y, float z) = 0;
	virtual void resetCamera() = 0;

	virtual void setCameraMode(CameraMode mode) = 0;
	virtual void setLightingFromIconSys(PS2IconSys* iconSys) = 0;
	virtual void setLightingMode(LightingMode mode) = 0;
	virtual void setBackgroundFromIconSys(PS2IconSys* iconSys) = 0;
	virtual void setBackgroundColor(float r, float g, float b, float a = 1.0f) = 0;

	virtual void render() = 0;
	virtual void resize(uint32_t width, uint32_t height) = 0;
	virtual void setVSync(bool enabled) { (void)enabled; }

	virtual uint32_t getVertexCount() const = 0;
	virtual uint32_t getFrameCount() const = 0;

protected:
	Error m_error;
};

class RendererFactory
{
public:
	enum class RendererType
	{
		Vulkan,
		OpenGL,
		Metal
	};

	static std::unique_ptr<Renderer> createRenderer(
		RendererType type,
		const WindowInfo& windowInfo,
		Config* config = nullptr);

	static std::unique_ptr<Renderer> createVulkanRenderer(
		const WindowInfo& windowInfo,
		Config* config = nullptr,
		Error* error = nullptr);

	static std::unique_ptr<Renderer> createOpenGLRenderer(
		const WindowInfo& windowInfo,
		Config* config = nullptr,
		Error* error = nullptr);

	static std::unique_ptr<Renderer> createMetalRenderer(
		const WindowInfo& windowInfo,
		Config* config = nullptr,
		Error* error = nullptr);

	static void registerRenderer(Renderer* renderer);
	static void unregisterRenderer(Renderer* renderer);
	static void applyVSyncToAll(bool enabled);
	static std::vector<std::string> getAvailableAdapters(RendererType type);

private:
	static std::vector<Renderer*> s_activeRenderers;
};
