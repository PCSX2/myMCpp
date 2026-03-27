// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "VulkanSwapchain.h"
#include "VulkanDevice.h"

#include <algorithm>
#include <array>
#include "Logger.h"
#include <vector>

VulkanSwapchain::VulkanSwapchain() = default;

VulkanSwapchain::~VulkanSwapchain() = default;

bool VulkanSwapchain::create(VulkanDevice& device, uint32_t width, uint32_t height)
{
	if (!createSwapchain(device, width, height))
		return false;
	if (!createImageViews(device.getDevice()))
		return false;
	if (!createDepthResources(device))
		return false;
	if (!createRenderPass(device.getDevice()))
		return false;
	if (!createFramebuffers(device.getDevice()))
		return false;
	return true;
}

void VulkanSwapchain::destroy(VkDevice device)
{
	Logger::info("VK: VulkanSwapchain::destroy start");
	for (auto framebuffer : m_framebuffers)
	{
		if (framebuffer != VK_NULL_HANDLE)
			vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
	m_framebuffers.clear();

	if (m_renderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(device, m_renderPass, nullptr);
		m_renderPass = VK_NULL_HANDLE;
	}

	destroyDepthResources(device);

	for (auto imageView : m_imageViews)
	{
		if (imageView != VK_NULL_HANDLE)
			vkDestroyImageView(device, imageView, nullptr);
	}
	m_imageViews.clear();

	Logger::info("VK: Destroying swapchain handle: {}", (void*)m_swapchain);
	if (m_swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
	}
	Logger::info("VK: VulkanSwapchain::destroy end");
}

bool VulkanSwapchain::recreate(VulkanDevice& device, uint32_t width, uint32_t height)
{
	vkDeviceWaitIdle(device.getDevice());
	destroy(device.getDevice());
	return create(device, width, height);
}

bool VulkanSwapchain::createSwapchain(VulkanDevice& device, uint32_t width, uint32_t height)
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.getPhysicalDevice(), device.getSurface(), &capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device.getPhysicalDevice(), device.getSurface(), &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device.getPhysicalDevice(), device.getSurface(), &formatCount, formats.data());

	VkSurfaceFormatKHR surfaceFormat = formats[0];
	for (const auto& fmt : formats)
	{
		if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			surfaceFormat = fmt;
			break;
		}
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device.getPhysicalDevice(), device.getSurface(), &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device.getPhysicalDevice(), device.getSurface(), &presentModeCount, presentModes.data());

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& mode : presentModes)
	{
		if (mode == VK_PRESENT_MODE_FIFO_KHR)
		{
			presentMode = mode;
			break;
		}
	}

	m_presentMode = presentMode;

	VkExtent2D extent = capabilities.currentExtent;
	if (extent.width == UINT32_MAX)
	{
		extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	}

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		imageCount = capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = device.getSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(device.getDevice(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create swapchain");
		return false;
	}

	vkGetSwapchainImagesKHR(device.getDevice(), m_swapchain, &imageCount, nullptr);
	m_images.resize(imageCount);
	vkGetSwapchainImagesKHR(device.getDevice(), m_swapchain, &imageCount, m_images.data());

	m_format = surfaceFormat.format;
	m_extent = extent;

	return true;
}

bool VulkanSwapchain::createImageViews(VkDevice device)
{
	m_imageViews.resize(m_images.size());
	for (size_t i = 0; i < m_images.size(); ++i)
	{
		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = m_images[i];
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = m_format;
		viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &viewCreateInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to create image view");
			return false;
		}
	}
	return true;
}

VkFormat VulkanSwapchain::findDepthFormat(VkPhysicalDevice physicalDevice)
{
	VkFormat candidates[] = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT};

	for (VkFormat format : candidates)
	{
		VkFormatProperties2 props{};
		props.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
		vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, &props);
		if (props.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			return format;
		}
	}
	return VK_FORMAT_D32_SFLOAT;
}

void VulkanSwapchain::destroyDepthResources(VkDevice device)
{
	for (auto v : m_depthImageViews)
	{
		if (v != VK_NULL_HANDLE)
			vkDestroyImageView(device, v, nullptr);
	}
	m_depthImageViews.clear();

	for (auto img : m_depthImages)
	{
		if (img != VK_NULL_HANDLE)
			vkDestroyImage(device, img, nullptr);
	}
	m_depthImages.clear();

	for (auto mem : m_depthMemories)
	{
		if (mem != VK_NULL_HANDLE)
			vkFreeMemory(device, mem, nullptr);
	}
	m_depthMemories.clear();
}

bool VulkanSwapchain::createDepthResources(VulkanDevice& device)
{
	destroyDepthResources(device.getDevice());

	m_depthFormat = findDepthFormat(device.getPhysicalDevice());

	const size_t n = m_images.size();
	m_depthImages.resize(n);
	m_depthMemories.resize(n);
	m_depthImageViews.resize(n);

	for (size_t i = 0; i < n; ++i)
	{
		m_depthImages[i] = VK_NULL_HANDLE;
		m_depthMemories[i] = VK_NULL_HANDLE;
		m_depthImageViews[i] = VK_NULL_HANDLE;

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = m_extent.width;
		imageInfo.extent.height = m_extent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = m_depthFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device.getDevice(), &imageInfo, nullptr, &m_depthImages[i]) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to create depth image");
			destroyDepthResources(device.getDevice());
			return false;
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device.getDevice(), m_depthImages[i], &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (allocInfo.memoryTypeIndex == UINT32_MAX)
		{
			Logger::error("VK: Failed to find suitable memory type for depth buffer");
			destroyDepthResources(device.getDevice());
			return false;
		}

		if (vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &m_depthMemories[i]) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to allocate depth image memory");
			destroyDepthResources(device.getDevice());
			return false;
		}

		vkBindImageMemory(device.getDevice(), m_depthImages[i], m_depthMemories[i], 0);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_depthImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_depthFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &m_depthImageViews[i]) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to create depth image view");
			destroyDepthResources(device.getDevice());
			return false;
		}
	}

	Logger::info("VK: Depth buffer created successfully");
	return true;
}

bool VulkanSwapchain::createRenderPass(VkDevice device)
{
	std::array<VkAttachmentDescription2, 2> attachments{};
	attachments[0].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	attachments[0].format = m_format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	attachments[1].format = m_depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference2 colorRef{};
	colorRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorRef.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkAttachmentReference2 depthRef{};
	depthRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthRef.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

	VkSubpassDescription2 subpass{};
	subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pDepthStencilAttachment = &depthRef;

	VkSubpassDependency2 dependency{};
	dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = 0;
	dependency.viewOffset = 0;

	VkRenderPassCreateInfo2 renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass2(device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create render pass");
		return false;
	}

	return true;
}

bool VulkanSwapchain::createFramebuffers(VkDevice device)
{
	m_framebuffers.resize(m_imageViews.size());

	for (size_t i = 0; i < m_imageViews.size(); ++i)
	{
		std::array<VkImageView, 2> attachments = {
			m_imageViews[i],
			m_depthImageViews[i]};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_extent.width;
		framebufferInfo.height = m_extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to create framebuffer");
			return false;
		}
	}

	return true;
}

bool VulkanSwapchain::setVSync(VulkanDevice& device, bool enabled)
{
	VkPresentModeKHR newPresentMode = enabled ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;

	if (newPresentMode == m_presentMode || m_swapchain == VK_NULL_HANDLE)
		return false;

	Logger::info("VK: Recreating swapchain with VSync {}", enabled ? "enabled" : "disabled");

	vkDeviceWaitIdle(device.getDevice());

	VkSwapchainKHR oldSwapchain = m_swapchain;

	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.getPhysicalDevice(), device.getSurface(), &capabilities);

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.surface = device.getSurface();
	createInfo.minImageCount = capabilities.minImageCount + 1;
	createInfo.imageFormat = m_format;
	createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	createInfo.imageExtent = m_extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = newPresentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapchain;

	VkSwapchainKHR newSwapchain;
	if (vkCreateSwapchainKHR(device.getDevice(), &createInfo, nullptr, &newSwapchain) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to recreate swapchain for VSync change");
		return false;
	}

	m_swapchain = newSwapchain;
	m_presentMode = newPresentMode;

	for (auto imageView : m_imageViews)
	{
		vkDestroyImageView(device.getDevice(), imageView, nullptr);
	}
	m_imageViews.clear();

	for (auto framebuffer : m_framebuffers)
	{
		vkDestroyFramebuffer(device.getDevice(), framebuffer, nullptr);
	}
	m_framebuffers.clear();

	destroyDepthResources(device.getDevice());

	uint32_t imageCount;
	vkGetSwapchainImagesKHR(device.getDevice(), m_swapchain, &imageCount, nullptr);
	m_images.resize(imageCount);
	vkGetSwapchainImagesKHR(device.getDevice(), m_swapchain, &imageCount, m_images.data());
	if (!createImageViews(device.getDevice()) || !createDepthResources(device) || !createFramebuffers(device.getDevice()))
	{
		Logger::error("VK: Failed to recreate swapchain resources for VSync change");
		return false;
	}

	if (oldSwapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device.getDevice(), oldSwapchain, nullptr);
	}
	return true;
}
