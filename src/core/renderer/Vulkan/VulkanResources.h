// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstdint>
#include <array>
#include <vector>
#include "common/Error.h"

class VulkanDevice;
struct VulkanBGVertex;

class VulkanResources
{
public:
	static constexpr uint32_t frameCount = 2;

	VulkanResources();
	~VulkanResources();

	VulkanResources(const VulkanResources&) = delete;
	VulkanResources& operator=(const VulkanResources&) = delete;

	bool createCommandPool(VkDevice device, uint32_t queueFamily);
	bool createSyncObjects(VkDevice device, uint32_t swapchainImageCount);
	bool recreateRenderFinishedSemaphores(VkDevice device, uint32_t swapchainImageCount);
	void destroy(VkDevice device, VmaAllocator allocator);

	const Error& GetError() const { return m_error; }

	VkCommandBuffer beginSingleTimeCommands(VkDevice device);
	void endSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandBuffer commandBuffer);

	bool createImage(VulkanDevice& device, uint32_t width, uint32_t height, VkFormat format,
		VkImageUsageFlags usage, float priority, VkImage& image, VmaAllocation& allocation);
	bool transitionImageLayout(VkDevice device, VkQueue queue, VkImage image, VkFormat format,
		VkImageLayout oldLayout, VkImageLayout newLayout);
	bool copyBufferToImage(VkDevice device, VkQueue queue, VkBuffer buffer, VkImage image,
		uint32_t width, uint32_t height);

	bool createMappedBuffer(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, float priority,
		VkBuffer& buffer, VmaAllocation& allocation, VmaAllocationInfo& mapped);

	bool createBackgroundVertexBuffer(VulkanDevice& device);
	void updateBackgroundVertexData(const VulkanBGVertex* vertices, size_t count);
	void destroyBackgroundVertexBuffer(VmaAllocator allocator);

	VkCommandPool getCommandPool() const { return m_commandPool; }
	VkSemaphore getImageAvailableSemaphore(uint32_t frameIndex) const { return m_imageAvailableSemaphores[frameIndex]; }
	VkSemaphore getRenderFinishedSemaphore(uint32_t imageIndex) const { return m_renderFinishedSemaphores[imageIndex]; }
	VkFence getInFlightFence(uint32_t frameIndex) const { return m_inFlightFences[frameIndex]; }
	VkBuffer getBackgroundVertexBuffer() const { return m_bgVertexBuffer; }

private:
	Error m_error;
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	std::array<VkSemaphore, frameCount> m_imageAvailableSemaphores{};
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::array<VkFence, frameCount> m_inFlightFences{};
	VkFence m_singleTimeFence = VK_NULL_HANDLE;

	VkBuffer m_bgVertexBuffer = VK_NULL_HANDLE;
	VmaAllocation m_bgVertexAllocation = VK_NULL_HANDLE;
	VmaAllocationInfo m_bgVertexAllocInfo{};
};
