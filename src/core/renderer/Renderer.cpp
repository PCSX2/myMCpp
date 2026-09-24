// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "Renderer.h"
#if defined(ENABLE_VULKAN)
#include "Vulkan/VulkanRenderer.h"
#include "Vulkan/VulkanDevice.h"
#endif
#if defined(ENABLE_OPENGL)
#include "OpenGL/OpenGLRenderer.h"
#endif
#if defined(ENABLE_METAL)
#include "Metal/MetalRenderer.h"
#endif
#include "common/Config.h"
#include "common/Logger.h"

std::vector<Renderer*> RendererFactory::s_activeRenderers;

RendererType RendererFactory::getAutomaticRendererType()
{
#if defined(ENABLE_METAL)
	return RendererType::Metal;
#elif defined(ENABLE_VULKAN)
	return RendererType::Vulkan;
#elif defined(ENABLE_OPENGL)
	return RendererType::OpenGL;
#else
	return RendererType::Automatic;
#endif
}

std::unique_ptr<Renderer> RendererFactory::createRenderer(
	RendererType type,
	const WindowInfo& windowInfo,
	Config* config,
	Error* error)
{
	const bool automatic = (type == RendererType::Automatic);
	if (automatic)
		type = getAutomaticRendererType();

	switch (type)
	{
		case RendererType::Vulkan:
#if defined(ENABLE_VULKAN)
			if (auto renderer = createVulkanRenderer(windowInfo, config, error))
				return renderer;
#endif
			if (automatic)
			{
#if defined(ENABLE_OPENGL)
				Logger::warn("Renderer: Vulkan failed to init, falling back to OpenGL: {}",
					(error && error->IsValid()) ? error->GetDescription() : "Unknown error");
				if (error)
					error->Clear();
				return createOpenGLRenderer(windowInfo, config, error);
#else
				Logger::warn("Renderer: Vulkan failed to init: {}",
					(error && error->IsValid()) ? error->GetDescription() : "Unknown error");
#endif
			}
			return nullptr;
		case RendererType::OpenGL:
#if defined(ENABLE_OPENGL)
			return createOpenGLRenderer(windowInfo, config, error);
#else
			return nullptr;
#endif
		case RendererType::Metal:
#if defined(ENABLE_METAL)
			if (auto renderer = createMetalRenderer(windowInfo, config, error))
				return renderer;
#endif
			if (automatic)
			{
#if defined(ENABLE_OPENGL)
				Logger::warn("Renderer: Metal failed to init, falling back to OpenGL: {}",
					(error && error->IsValid()) ? error->GetDescription() : "Unknown error");
				if (error)
					error->Clear();
				return createOpenGLRenderer(windowInfo, config, error);
#else
				Logger::warn("Renderer: Metal failed to init: {}",
					(error && error->IsValid()) ? error->GetDescription() : "Unknown error");
#endif
			}
			return nullptr;
		case RendererType::Automatic:
			break;
	}

	return nullptr;
}

#if defined(ENABLE_VULKAN)
std::unique_ptr<Renderer> RendererFactory::createVulkanRenderer(const WindowInfo& windowInfo, Config* config, Error* error)
{
	auto renderer = std::make_unique<VulkanRenderer>(windowInfo, config);
	if (!renderer->initialize())
	{
		if (error)
			*error = renderer->GetError();
		return nullptr;
	}
	return renderer;
}
#endif

#if defined(ENABLE_OPENGL)
std::unique_ptr<Renderer> RendererFactory::createOpenGLRenderer(const WindowInfo& windowInfo, Config* config, Error* error)
{
	auto renderer = std::make_unique<OpenGLRenderer>(windowInfo, config);
	if (!renderer->initialize())
	{
		if (error)
			*error = renderer->GetError();
		return nullptr;
	}
	return renderer;
}
#endif

#if defined(ENABLE_METAL)
std::unique_ptr<Renderer> RendererFactory::createMetalRenderer(const WindowInfo& windowInfo, Config* config, Error* error)
{
	auto renderer = std::make_unique<MetalRenderer>(windowInfo, config);
	if (!renderer->initialize())
	{
		if (error)
			*error = renderer->GetError();
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

std::vector<std::string> RendererFactory::getAvailableAdapters(RendererType type)
{
	if (type == RendererType::Automatic)
		type = getAutomaticRendererType();

	// Chose switch because in the near future I plan to have D3D renderers.
	switch (type)
	{
		case RendererType::Vulkan:
#if defined(ENABLE_VULKAN)
			return VulkanDevice::getAvailableAdapters();
#else
			return {};
#endif
		case RendererType::Automatic:
		case RendererType::OpenGL:
			return {};
		case RendererType::Metal:
#if defined(ENABLE_METAL)
			return MetalRenderer::getAvailableAdapters();
#else
			return {};
#endif
	}

	return {};
}
