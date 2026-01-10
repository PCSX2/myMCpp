// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "Renderer.h"
#if !defined(__APPLE__)
#include "Vulkan/VulkanRenderer.h"
#endif
#include "OpenGL/OpenGLRenderer.h"
#if defined(__APPLE__)
#include "Metal/MetalRenderer.h"
#endif

std::unique_ptr<Renderer> RendererFactory::createRenderer(
	RendererType type,
	const WindowInfo& windowInfo)
{
	switch (type)
	{
		case RendererType::Vulkan:
#if !defined(__APPLE__)
			return createVulkanRenderer(windowInfo);
#endif
		case RendererType::OpenGL:
			return createOpenGLRenderer(windowInfo);
		case RendererType::Metal:
#if defined(__APPLE__)
			return createMetalRenderer(windowInfo);
#endif
		default:
			return nullptr;
	}
}

#if !defined(__APPLE__)
std::unique_ptr<Renderer> RendererFactory::createVulkanRenderer(const WindowInfo& windowInfo)
{
	auto renderer = std::make_unique<VulkanRenderer>(windowInfo);
	if (!renderer->initialize())
	{
		return nullptr;
	}
	return renderer;
}
#endif

std::unique_ptr<Renderer> RendererFactory::createOpenGLRenderer(const WindowInfo& windowInfo)
{
	auto renderer = std::make_unique<OpenGLRenderer>(windowInfo);
	if (!renderer->initialize())
	{
		return nullptr;
	}
	return renderer;
}

#if defined(__APPLE__)
std::unique_ptr<Renderer> RendererFactory::createMetalRenderer(const WindowInfo& windowInfo)
{
	auto renderer = std::make_unique<MetalRenderer>(windowInfo);
	if (!renderer->initialize())
	{
		return nullptr;
	}
	return renderer;
}
#endif
