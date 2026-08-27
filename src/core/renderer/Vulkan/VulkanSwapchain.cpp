// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "VulkanSwapchain.h"
#include "VulkanDevice.h"

#include <vk_mem_alloc.h>

#include <algorithm>
#include <array>
#include "common/Logger.h"

VulkanSwapchain::VulkanSwapchain() = default;

VulkanSwapchain::~VulkanSwapchain() = default;

bool VulkanSwapchain::create(VulkanDevice& device, uint32_t width, uint32_t height, bool vsync)
{
	m_error.Clear();

	m_presentMode = vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;

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

void VulkanSwapchain::destroy(VkDevice device, VmaAllocator allocator)
{
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

	destroyDepthResources(device, allocator);

	for (auto imageView : m_imageViews)
	{
		if (imageView != VK_NULL_HANDLE)
			vkDestroyImageView(device, imageView, nullptr);
	}
	m_imageViews.clear();

	if (m_swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
	}
}

bool VulkanSwapchain::recreate(VulkanDevice& device, uint32_t width, uint32_t height)
{
	if (m_swapchain == VK_NULL_HANDLE)
		return create(device, width, height);

	VkSwapchainKHR oldSwapchain = m_swapchain;

	if (!createSwapchain(device, width, height, oldSwapchain))
		return false;

	if (!rebuildAttachments(device, oldSwapchain))
	{
		Logger::error("VK: Failed to recreate swapchain resources");
		return false;
	}
	return true;
}

bool VulkanSwapchain::rebuildAttachments(VulkanDevice& device, VkSwapchainKHR oldSwapchain)
{
	VkDevice vkDevice = device.getDevice();

	for (auto imageView : m_imageViews)
		vkDestroyImageView(vkDevice, imageView, nullptr);
	m_imageViews.clear();

	for (auto framebuffer : m_framebuffers)
		vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
	m_framebuffers.clear();

	destroyDepthResources(vkDevice, device.getAllocator());

	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(vkDevice, m_swapchain, &imageCount, nullptr);
	m_images.resize(imageCount);
	vkGetSwapchainImagesKHR(vkDevice, m_swapchain, &imageCount, m_images.data());

	if (!createImageViews(vkDevice) || !createDepthResources(device) || !createFramebuffers(vkDevice))
		return false;

	if (oldSwapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(vkDevice, oldSwapchain, nullptr);
	return true;
}

bool VulkanSwapchain::createSwapchain(VulkanDevice& device, uint32_t width, uint32_t height,
	VkSwapchainKHR oldSwapchain)
{
	VkSurfaceCapabilitiesKHR capabilities;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.getPhysicalDevice(), device.getSurface(), &capabilities) != VK_SUCCESS)
		return m_error.Fail("VK: Failed to get physical device surface capabilities");

	uint32_t formatCount = 0;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(device.getPhysicalDevice(), device.getSurface(), &formatCount, nullptr) != VK_SUCCESS || formatCount == 0)
		return m_error.Fail("VK: Failed to get physical device surface formats or none supported");

	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(device.getPhysicalDevice(), device.getSurface(), &formatCount, formats.data()) != VK_SUCCESS || formats.empty())
		return m_error.Fail("VK: Failed to retrieve physical device surface formats");

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
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(device.getPhysicalDevice(), device.getSurface(), &presentModeCount, nullptr) != VK_SUCCESS || presentModeCount == 0)
		return m_error.Fail("VK: Failed to get physical device surface present modes or none supported");

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(device.getPhysicalDevice(), device.getSurface(), &presentModeCount, presentModes.data()) != VK_SUCCESS || presentModes.empty())
		return m_error.Fail("VK: Failed to retrieve physical device surface present modes");

	VkPresentModeKHR presentMode = m_presentMode;
	bool supported = false;
	for (const auto& mode : presentModes)
	{
		if (mode == presentMode)
		{
			supported = true;
			break;
		}
	}
	if (!supported)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			for (const auto& mode : presentModes)
			{
				// If mailbox is not supported, try immediate.
				if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					presentMode = mode;
					supported = true;
					Logger::warn("VK: Mailbox present mode not supported, falling back to immediate");
					break;
				}
			}
		}
		// If mailbox and immediate are not supported, use fifo.
		if (!supported)
		{
			if (m_presentMode != VK_PRESENT_MODE_FIFO_KHR)
				Logger::warn("VK: Requested present mode not supported, falling back to FIFO");
			presentMode = VK_PRESENT_MODE_FIFO_KHR;
		}
	}

	m_presentMode = presentMode;

	VkExtent2D extent = capabilities.currentExtent;
	if (extent.width == UINT32_MAX || extent.width == 0 || extent.height == 0
#if defined(__APPLE__)
		|| (width > 0 && height > 0 && (extent.width != width || extent.height != height))
#endif
	)
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
	createInfo.oldSwapchain = oldSwapchain;

	if (vkCreateSwapchainKHR(device.getDevice(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
		return m_error.Fail("VK: Failed to create swapchain");

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
			return m_error.Fail("VK: Failed to create image view");
	}
	return true;
}

VkFormat VulkanSwapchain::findDepthFormat(VkPhysicalDevice physicalDevice)
{
	VkFormat candidates[] = {
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT};

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
	return VK_FORMAT_D24_UNORM_S8_UINT;
}

void VulkanSwapchain::destroyDepthResources(VkDevice device, VmaAllocator allocator)
{
	for (auto v : m_depthImageViews)
	{
		if (v != VK_NULL_HANDLE)
			vkDestroyImageView(device, v, nullptr);
	}
	m_depthImageViews.clear();

	for (size_t i = 0; i < m_depthImages.size(); ++i)
	{
		if (m_depthImages[i] != VK_NULL_HANDLE)
			vmaDestroyImage(allocator, m_depthImages[i], m_depthAllocations[i]);
	}
	m_depthImages.clear();
	m_depthAllocations.clear();
}

bool VulkanSwapchain::createDepthResources(VulkanDevice& device)
{
	destroyDepthResources(device.getDevice(), device.getAllocator());

	m_depthFormat = findDepthFormat(device.getPhysicalDevice());

	const size_t n = m_images.size();
	m_depthImages.resize(n);
	m_depthAllocations.resize(n);
	m_depthImageViews.resize(n);

	for (size_t i = 0; i < n; ++i)
	{
		m_depthImages[i] = VK_NULL_HANDLE;
		m_depthAllocations[i] = VK_NULL_HANDLE;
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

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		device.setAllocationPriority(allocInfo, 1.0f);

		if (vmaCreateImage(device.getAllocator(), &imageInfo, &allocInfo, &m_depthImages[i], &m_depthAllocations[i], nullptr) != VK_SUCCESS)
		{
			destroyDepthResources(device.getDevice(), device.getAllocator());
			return m_error.Fail("VK: Failed to create depth image");
		}

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
			destroyDepthResources(device.getDevice(), device.getAllocator());
			return m_error.Fail("VK: Failed to create depth image view");
		}
	}

	return true;
}

bool VulkanSwapchain::createRenderPass(VkDevice device)
{
	std::array<VkAttachmentDescription, 2> attachments{};
	attachments[0].format = m_format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1].format = m_depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorRef{};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthRef{};
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pDepthStencilAttachment = &depthRef;

	std::array<VkSubpassDependency, 2> dependencies{};
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = 0;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = 0;
	dependencies[1].dependencyFlags = 0;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		return m_error.Fail("VK: Failed to create render pass");

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
			return m_error.Fail("VK: Failed to create framebuffer");
	}

	return true;
}

bool VulkanSwapchain::setVSync(VulkanDevice& device, bool enabled)
{
	VkPresentModeKHR newPresentMode = enabled ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;

	if (newPresentMode == m_presentMode || m_swapchain == VK_NULL_HANDLE)
		return false;

	Logger::info("VK: VSync {}", enabled ? "enabled" : "disabled");

	m_presentMode = newPresentMode;

	VkSwapchainKHR oldSwapchain = m_swapchain;
	if (!createSwapchain(device, m_extent.width, m_extent.height, oldSwapchain))
	{
		Logger::error("VK: Failed to recreate swapchain for VSync change");
		return false;
	}

	if (!rebuildAttachments(device, oldSwapchain))
	{
		Logger::error("VK: Failed to recreate swapchain resources for VSync change");
		return false;
	}
	return true;
}
