// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstdint>
#include "../../../common/Error.h"

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

	const Error& GetError() const { return m_error; }

	VkInstance getInstance() const { return m_instance; }
	VkSurfaceKHR getSurface() const { return m_surface; }
	VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
	VkDevice getDevice() const { return m_device; }
	VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
	uint32_t getGraphicsQueueFamily() const { return m_graphicsQueueFamily; }
	VmaAllocator getAllocator() const { return m_allocator; }
	bool supportsMemoryPriority() const { return m_supportsMemoryPriority; }

	void setAllocationPriority(VmaAllocationCreateInfo& allocInfo, float priority) const;

private:
	bool createInstance();
	bool createSurface(const WindowInfo& windowInfo);
	bool selectPhysicalDevice();
	bool createLogicalDevice();

	Error m_error;

	VkInstance m_instance = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	uint32_t m_graphicsQueueFamily = 0;
	VmaAllocator m_allocator = VK_NULL_HANDLE;
	bool m_supportsMemoryPriority = false;
#ifdef __linux__
	void* m_platformDisplay = nullptr; // Display* on Linux/X11
#endif
#ifndef NDEBUG
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif
};
