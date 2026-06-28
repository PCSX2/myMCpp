// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"

#include <cstring>
#include "Logger.h"

VulkanResources::VulkanResources() = default;

VulkanResources::~VulkanResources() = default;

bool VulkanResources::createCommandPool(VkDevice device, uint32_t queueFamily)
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamily;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create command pool");
		return false;
	}

	return true;
}

bool VulkanResources::recreateRenderFinishedSemaphores(VkDevice device, uint32_t swapchainImageCount)
{
	for (VkSemaphore s : m_renderFinishedSemaphores)
	{
		if (s != VK_NULL_HANDLE)
			vkDestroySemaphore(device, s, nullptr);
	}
	m_renderFinishedSemaphores.clear();
	m_renderFinishedSemaphores.resize(swapchainImageCount);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (uint32_t i = 0; i < swapchainImageCount; ++i)
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to create render-finished semaphore {}", i);
			for (uint32_t j = 0; j < i; ++j)
			{
				vkDestroySemaphore(device, m_renderFinishedSemaphores[j], nullptr);
				m_renderFinishedSemaphores[j] = VK_NULL_HANDLE;
			}
			m_renderFinishedSemaphores.clear();
			return false;
		}
	}
	return true;
}

bool VulkanResources::createSyncObjects(VkDevice device, uint32_t swapchainImageCount)
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < frameCount; ++i)
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to create synchronization objects for frame {}", i);
			return false;
		}
	}

	if (!recreateRenderFinishedSemaphores(device, swapchainImageCount))
		return false;

	VkFenceCreateInfo singleTimeFenceInfo{};
	singleTimeFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	singleTimeFenceInfo.flags = 0;

	if (vkCreateFence(device, &singleTimeFenceInfo, nullptr, &m_singleTimeFence) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create single-time command fence");
		return false;
	}

	Logger::info("VK: Created sync objects for {} frames", frameCount);
	return true;
}

void VulkanResources::destroy(VkDevice device, VmaAllocator allocator)
{
	destroyBackgroundVertexBuffer(allocator);

	if (m_singleTimeFence != VK_NULL_HANDLE)
	{
		vkDestroyFence(device, m_singleTimeFence, nullptr);
		m_singleTimeFence = VK_NULL_HANDLE;
	}

	for (VkSemaphore s : m_renderFinishedSemaphores)
	{
		if (s != VK_NULL_HANDLE)
			vkDestroySemaphore(device, s, nullptr);
	}
	m_renderFinishedSemaphores.clear();

	for (uint32_t i = 0; i < frameCount; ++i)
	{
		if (m_inFlightFences[i] != VK_NULL_HANDLE)
		{
			vkDestroyFence(device, m_inFlightFences[i], nullptr);
			m_inFlightFences[i] = VK_NULL_HANDLE;
		}
		if (m_imageAvailableSemaphores[i] != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(device, m_imageAvailableSemaphores[i], nullptr);
			m_imageAvailableSemaphores[i] = VK_NULL_HANDLE;
		}
	}

	if (m_commandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(device, m_commandPool, nullptr);
		m_commandPool = VK_NULL_HANDLE;
	}
}

VkCommandBuffer VulkanResources::beginSingleTimeCommands(VkDevice device)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void VulkanResources::endSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkResetFences(device, 1, &m_singleTimeFence);
	vkQueueSubmit(queue, 1, &submitInfo, m_singleTimeFence);
	vkWaitForFences(device, 1, &m_singleTimeFence, VK_TRUE, UINT64_MAX);

	vkFreeCommandBuffers(device, m_commandPool, 1, &commandBuffer);
}

bool VulkanResources::createImage(VulkanDevice& device, uint32_t width, uint32_t height, VkFormat format,
	VkImageUsageFlags usage, float priority, VkImage& image, VmaAllocation& allocation)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	device.setAllocationPriority(allocInfo, priority);

	if (vmaCreateImage(device.getAllocator(), &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create image");
		return false;
	}

	return true;
}

bool VulkanResources::transitionImageLayout(VkDevice device, VkQueue queue, VkImage image, VkFormat /*format*/,
	VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else
	{
		Logger::error("VK: Unsupported layout transition");
		endSingleTimeCommands(device, queue, commandBuffer);
		return false;
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	endSingleTimeCommands(device, queue, commandBuffer);
	return true;
}

bool VulkanResources::copyBufferToImage(VkDevice device, VkQueue queue, VkBuffer buffer, VkImage image,
	uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(device, queue, commandBuffer);
	return true;
}

bool VulkanResources::createMappedBuffer(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, float priority,
	VkBuffer& buffer, VmaAllocation& allocation, VmaAllocationInfo& mapped)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	device.setAllocationPriority(allocInfo, priority);

	return vmaCreateBuffer(device.getAllocator(), &bufferInfo, &allocInfo, &buffer, &allocation, &mapped) == VK_SUCCESS;
}

bool VulkanResources::createBackgroundVertexBuffer(VulkanDevice& device)
{
	destroyBackgroundVertexBuffer(device.getAllocator());

	if (!createMappedBuffer(device, sizeof(VulkanBGVertex) * 4, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0.5f,
			m_bgVertexBuffer, m_bgVertexAllocation, m_bgVertexAllocInfo))
	{
		Logger::error("VK: Failed to create background vertex buffer");
		return false;
	}

	return true;
}

void VulkanResources::updateBackgroundVertexData(const VulkanBGVertex* vertices, size_t count)
{
	if (m_bgVertexAllocation == VK_NULL_HANDLE || !vertices || count == 0)
		return;

	std::memcpy(m_bgVertexAllocInfo.pMappedData, vertices, sizeof(VulkanBGVertex) * count);
}

void VulkanResources::destroyBackgroundVertexBuffer(VmaAllocator allocator)
{
	if (m_bgVertexBuffer != VK_NULL_HANDLE)
	{
		vmaDestroyBuffer(allocator, m_bgVertexBuffer, m_bgVertexAllocation);
		m_bgVertexBuffer = VK_NULL_HANDLE;
		m_bgVertexAllocation = VK_NULL_HANDLE;
	}
}
