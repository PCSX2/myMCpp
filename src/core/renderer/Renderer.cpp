// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "Renderer.h"
#if !defined(__APPLE__)
#include "Vulkan/VulkanRenderer.h"
#endif
#include "OpenGL/OpenGLRenderer.h"
#if defined(__APPLE__)
#include "Metal/MetalRenderer.h"
#endif
#include "../../common/Config.h"

std::vector<Renderer*> RendererFactory::s_activeRenderers;

std::unique_ptr<Renderer> RendererFactory::createRenderer(
	RendererType type,
	const WindowInfo& windowInfo,
	Config* config)
{
	switch (type)
	{
		case RendererType::Vulkan:
#if !defined(__APPLE__)
			return createVulkanRenderer(windowInfo, config);
#endif
		case RendererType::OpenGL:
			return createOpenGLRenderer(windowInfo, config);
		case RendererType::Metal:
#if defined(__APPLE__)
			return createMetalRenderer(windowInfo, config);
#endif
		default:
			return nullptr;
	}
}

#if !defined(__APPLE__)
std::unique_ptr<Renderer> RendererFactory::createVulkanRenderer(const WindowInfo& windowInfo, Config* config)
{
	auto renderer = std::make_unique<VulkanRenderer>(windowInfo, config);
	if (!renderer->initialize())
	{
		return nullptr;
	}
	return renderer;
}
#endif

std::unique_ptr<Renderer> RendererFactory::createOpenGLRenderer(const WindowInfo& windowInfo, Config* config)
{
	auto renderer = std::make_unique<OpenGLRenderer>(windowInfo, config);
	if (!renderer->initialize())
	{
		return nullptr;
	}
	return renderer;
}

#if defined(__APPLE__)
std::unique_ptr<Renderer> RendererFactory::createMetalRenderer(const WindowInfo& windowInfo, Config* config)
{
	auto renderer = std::make_unique<MetalRenderer>(windowInfo, config);
	if (!renderer->initialize())
	{
		return nullptr;
	}
	return renderer;
}
#endif

void RendererFactory::registerRenderer(Renderer* renderer)
{
	if (renderer && std::find(s_activeRenderers.begin(), s_activeRenderers.end(), renderer) == s_activeRenderers.end())
	{
		s_activeRenderers.push_back(renderer);
	}
}

void RendererFactory::unregisterRenderer(Renderer* renderer)
{
	auto it = std::find(s_activeRenderers.begin(), s_activeRenderers.end(), renderer);
	if (it != s_activeRenderers.end())
	{
		s_activeRenderers.erase(it);
	}
}

void RendererFactory::applyVSyncToAll(bool enabled)
{
	for (Renderer* renderer : s_activeRenderers)
	{
		if (renderer)
		{
			renderer->setVSync(enabled);
		}
	}
}
