// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstdint>

struct WindowInfo;

class VulkanDevice
{
public:
	VulkanDevice();
	~VulkanDevice();

	VulkanDevice(const VulkanDevice&) = delete;
	VulkanDevice& operator=(const VulkanDevice&) = delete;

	bool create(const WindowInfo& windowInfo);
	void destroy();

	VkInstance getInstance() const { return m_instance; }
	VkSurfaceKHR getSurface() const { return m_surface; }
	VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
	VkDevice getDevice() const { return m_device; }
	VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
	uint32_t getGraphicsQueueFamily() const { return m_graphicsQueueFamily; }
	VmaAllocator getAllocator() const { return m_allocator; }

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

private:
	bool createInstance();
	bool createSurface(const WindowInfo& windowInfo);
	bool selectPhysicalDevice();
	bool createLogicalDevice();

	VkInstance m_instance = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	uint32_t m_graphicsQueueFamily = 0;
	VmaAllocator m_allocator = VK_NULL_HANDLE;
#ifdef __linux__
	void* m_platformDisplay = nullptr; // Display* on Linux/X11
#endif
};
