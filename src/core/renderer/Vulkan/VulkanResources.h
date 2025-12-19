// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

class VulkanDevice;
struct VulkanBGVertex;

class VulkanResources
{
public:
	VulkanResources();
	~VulkanResources();

	VulkanResources(const VulkanResources&) = delete;
	VulkanResources& operator=(const VulkanResources&) = delete;

	bool createCommandPool(VkDevice device, uint32_t queueFamily);
	bool createSyncObjects(VkDevice device);
	void destroy(VkDevice device);

	VkCommandBuffer beginSingleTimeCommands(VkDevice device);
	void endSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandBuffer commandBuffer);

	bool createImage(VulkanDevice& device, uint32_t width, uint32_t height, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkImage& image, VkDeviceMemory& imageMemory);
	bool transitionImageLayout(VkDevice device, VkQueue queue, VkImage image, VkFormat format,
		VkImageLayout oldLayout, VkImageLayout newLayout);
	bool copyBufferToImage(VkDevice device, VkQueue queue, VkBuffer buffer, VkImage image,
		uint32_t width, uint32_t height);

	bool createBackgroundVertexBuffer(VulkanDevice& device);
	void updateBackgroundVertexData(VkDevice device, const VulkanBGVertex* vertices, size_t count);
	void destroyBackgroundVertexBuffer(VkDevice device);

	VkCommandPool getCommandPool() const { return m_commandPool; }
	VkSemaphore getImageAvailableSemaphore() const { return m_imageAvailableSemaphore; }
	VkSemaphore getRenderFinishedSemaphore() const { return m_renderFinishedSemaphore; }
	VkFence getInFlightFence() const { return m_inFlightFence; }
	VkBuffer getBackgroundVertexBuffer() const { return m_bgVertexBuffer; }

private:
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
	VkSemaphore m_renderFinishedSemaphore = VK_NULL_HANDLE;
	VkFence m_inFlightFence = VK_NULL_HANDLE;

	VkBuffer m_bgVertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_bgVertexMemory = VK_NULL_HANDLE;
};
