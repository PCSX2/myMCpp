// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstdint>
#include <vector>
#include "../../../common/Error.h"

class VulkanDevice;

class VulkanSwapchain
{
public:
	VulkanSwapchain();
	~VulkanSwapchain();

	VulkanSwapchain(const VulkanSwapchain&) = delete;
	VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

	bool create(VulkanDevice& device, uint32_t width, uint32_t height);
	void destroy(VkDevice device, VmaAllocator allocator);
	bool recreate(VulkanDevice& device, uint32_t width, uint32_t height);

	const Error& GetError() const { return m_error; }

	VkSwapchainKHR getSwapchain() const { return m_swapchain; }
	VkRenderPass getRenderPass() const { return m_renderPass; }
	VkFormat getFormat() const { return m_format; }
	VkExtent2D getExtent() const { return m_extent; }
	const std::vector<VkImageView>& getImageViews() const { return m_imageViews; }
	const std::vector<VkImage>& getImages() const { return m_images; }
	const std::vector<VkFramebuffer>& getFramebuffers() const { return m_framebuffers; }
	VkFormat getDepthFormat() const { return m_depthFormat; }

	bool setVSync(VulkanDevice& device, bool enabled);

private:
	bool createSwapchain(VulkanDevice& device, uint32_t width, uint32_t height,
		VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
	bool rebuildAttachments(VulkanDevice& device, VkSwapchainKHR oldSwapchain);
	bool createImageViews(VkDevice device);
	bool createDepthResources(VulkanDevice& device);
	bool createRenderPass(VkDevice device);
	bool createFramebuffers(VkDevice device);
	void destroyDepthResources(VkDevice device, VmaAllocator allocator);
	VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

	Error m_error;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;
	VkFormat m_format = VK_FORMAT_UNDEFINED;
	VkExtent2D m_extent = {0, 0};

	std::vector<VkImage> m_depthImages;
	std::vector<VmaAllocation> m_depthAllocations;
	std::vector<VkImageView> m_depthImageViews;
	VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;

	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> m_framebuffers;

	VkPresentModeKHR m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
};
