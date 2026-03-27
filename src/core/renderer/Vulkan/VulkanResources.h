// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <array>
#include <vector>
#include "Logger.h"

class VulkanDevice;
struct VulkanBGVertex;

static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

#define VK_CHECK(call) \
	do \
	{ \
		VkResult result = call; \
		if (result != VK_SUCCESS) \
		{ \
			Logger::error("VK: {} failed with error {} at {}:{}", #call, static_cast<int>(result), __FILE__, __LINE__); \
		} \
	} while (0)

#define VK_CHECK_RETURN(call) \
	do \
	{ \
		VkResult result = call; \
		if (result != VK_SUCCESS) \
		{ \
			Logger::error("VK: {} failed with error {} at {}:{}", #call, static_cast<int>(result), __FILE__, __LINE__); \
			return false; \
		} \
	} while (0)

class VulkanResources
{
public:
	VulkanResources();
	~VulkanResources();

	VulkanResources(const VulkanResources&) = delete;
	VulkanResources& operator=(const VulkanResources&) = delete;

	bool createCommandPool(VkDevice device, uint32_t queueFamily);
	bool createSyncObjects(VkDevice device, uint32_t swapchainImageCount);
	bool recreateRenderFinishedSemaphores(VkDevice device, uint32_t swapchainImageCount);
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
	VkSemaphore getImageAvailableSemaphore(uint32_t frameIndex) const { return m_imageAvailableSemaphores[frameIndex]; }
	VkSemaphore getRenderFinishedSemaphore(uint32_t imageIndex) const { return m_renderFinishedSemaphores[imageIndex]; }
	VkFence getInFlightFence(uint32_t frameIndex) const { return m_inFlightFences[frameIndex]; }
	VkBuffer getBackgroundVertexBuffer() const { return m_bgVertexBuffer; }

private:
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_imageAvailableSemaphores{};
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_inFlightFences{};
	VkFence m_singleTimeFence = VK_NULL_HANDLE;

	VkBuffer m_bgVertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_bgVertexMemory = VK_NULL_HANDLE;
};
