// SPDX-FileCopyrightText: 2025 SternXD
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

bool VulkanResources::createSyncObjects(VkDevice device)
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateFence(device, &fenceInfo, nullptr, &m_inFlightFence) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create synchronization objects");
		return false;
	}

	return true;
}

void VulkanResources::destroy(VkDevice device)
{
	destroyBackgroundVertexBuffer(device);

	if (m_inFlightFence != VK_NULL_HANDLE)
	{
		vkDestroyFence(device, m_inFlightFence, nullptr);
		m_inFlightFence = VK_NULL_HANDLE;
	}
	if (m_renderFinishedSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(device, m_renderFinishedSemaphore, nullptr);
		m_renderFinishedSemaphore = VK_NULL_HANDLE;
	}
	if (m_imageAvailableSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(device, m_imageAvailableSemaphore, nullptr);
		m_imageAvailableSemaphore = VK_NULL_HANDLE;
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

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, m_commandPool, 1, &commandBuffer);
}

bool VulkanResources::createImage(VulkanDevice& device, uint32_t width, uint32_t height, VkFormat format,
	VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
	VkImage& image, VkDeviceMemory& imageMemory)
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
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	if (vkCreateImage(device.getDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create image");
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device.getDevice(), image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties);

	if (allocInfo.memoryTypeIndex == UINT32_MAX || vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to allocate image memory");
		vkDestroyImage(device.getDevice(), image, nullptr);
		image = VK_NULL_HANDLE;
		return false;
	}

	vkBindImageMemory(device.getDevice(), image, imageMemory, 0);
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

bool VulkanResources::createBackgroundVertexBuffer(VulkanDevice& device)
{
	destroyBackgroundVertexBuffer(device.getDevice());

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(VulkanBGVertex) * 4;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device.getDevice(), &bufferInfo, nullptr, &m_bgVertexBuffer) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create background vertex buffer");
		return false;
	}

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(device.getDevice(), m_bgVertexBuffer, &memReq);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = device.findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (allocInfo.memoryTypeIndex == UINT32_MAX || vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &m_bgVertexMemory) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to allocate background vertex memory");
		vkDestroyBuffer(device.getDevice(), m_bgVertexBuffer, nullptr);
		m_bgVertexBuffer = VK_NULL_HANDLE;
		return false;
	}

	vkBindBufferMemory(device.getDevice(), m_bgVertexBuffer, m_bgVertexMemory, 0);
	return true;
}

void VulkanResources::updateBackgroundVertexData(VkDevice device, const VulkanBGVertex* vertices, size_t count)
{
	if (m_bgVertexMemory == VK_NULL_HANDLE || !vertices || count == 0)
		return;

	void* data = nullptr;
	if (vkMapMemory(device, m_bgVertexMemory, 0, sizeof(VulkanBGVertex) * count, 0, &data) == VK_SUCCESS)
	{
		std::memcpy(data, vertices, sizeof(VulkanBGVertex) * count);
		vkUnmapMemory(device, m_bgVertexMemory);
	}
}

void VulkanResources::destroyBackgroundVertexBuffer(VkDevice device)
{
	if (m_bgVertexBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, m_bgVertexBuffer, nullptr);
		m_bgVertexBuffer = VK_NULL_HANDLE;
	}
	if (m_bgVertexMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, m_bgVertexMemory, nullptr);
		m_bgVertexMemory = VK_NULL_HANDLE;
	}
}
