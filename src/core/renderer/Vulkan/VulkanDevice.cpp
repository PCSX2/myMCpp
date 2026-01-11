// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "VulkanDevice.h"
#include "WindowInfo.h"

#if defined(_WIN32)
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#elif defined(__linux__)
#include "../Renderer.h"
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#include <wayland-client.h>
#include <vulkan/vulkan_wayland.h>
#endif

#include "Logger.h"
#include <vector>

VulkanDevice::VulkanDevice() = default;

VulkanDevice::~VulkanDevice()
{
	destroy();
}

bool VulkanDevice::create(const WindowInfo& windowInfo)
{
	if (!createInstance())
		return false;
	if (!createSurface(windowInfo))
		return false;
	if (!selectPhysicalDevice())
		return false;
	if (!createLogicalDevice())
		return false;
	return true;
}

void VulkanDevice::destroy()
{
	if (m_allocator != VK_NULL_HANDLE)
	{
		vmaDestroyAllocator(m_allocator);
		m_allocator = VK_NULL_HANDLE;
	}
	if (m_device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
	}
	if (m_surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
	}
	if (m_instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	}

#ifdef __linux__
	if (m_platformDisplay)
	{
		XCloseDisplay(static_cast<Display*>(m_platformDisplay));
		m_platformDisplay = nullptr;
	}
#endif
}

bool VulkanDevice::createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "myMCpp";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "myMCpp";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	std::vector<const char*> extensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(__linux__)
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
		VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
	};

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create Vulkan instance");
		return false;
	}

	return true;
}

bool VulkanDevice::createSurface(const WindowInfo& windowInfo)
{
	if (windowInfo.type != WindowInfo::Type::Surfaceless && !windowInfo.window_handle)
	{
		Logger::error("VK: No native window handle available");
		return false;
	}

#if defined(_WIN32)
	if (windowInfo.type != WindowInfo::Type::Win32)
	{
		Logger::error("VK: Invalid window type for Windows");
		return false;
	}

	HWND hwnd = reinterpret_cast<HWND>(windowInfo.window_handle);

	if (!IsWindow(hwnd))
	{
		Logger::error("VK: Window handle is invalid");
		return false;
	}

	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = hwnd;
	createInfo.hinstance = GetModuleHandle(nullptr);

	VkResult result = vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);

	if (result != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create window surface: {}", static_cast<int>(result));
		return false;
	}

	Logger::info("VK: Win32 surface created successfully");
	return true;
#elif defined(__linux__)
	if (windowInfo.type == WindowInfo::Type::Wayland)
	{
		VkWaylandSurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
		createInfo.display = static_cast<wl_display*>(windowInfo.display_connection);
		createInfo.surface = static_cast<wl_surface*>(windowInfo.window_handle);

		if (vkCreateWaylandSurfaceKHR(m_instance, &createInfo, nullptr, &m_surface) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to create Wayland surface");
			return false;
		}
		Logger::info("VK: Wayland surface created successfully");
		return true;
	}
	else if (windowInfo.type == WindowInfo::Type::X11)
	{
		Display* display = static_cast<Display*>(windowInfo.display_connection);
		bool ownDisplay = false;
		
		if (!display)
		{
			display = XOpenDisplay(nullptr);
			if (!display)
			{
				Logger::error("VK: Failed to open X display");
				return false;
			}
			ownDisplay = true;
		}

		VkXlibSurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
		createInfo.dpy = display;
		createInfo.window = reinterpret_cast<Window>(windowInfo.window_handle);

		if (vkCreateXlibSurfaceKHR(m_instance, &createInfo, nullptr, &m_surface) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to create Xlib surface");
			if (ownDisplay) XCloseDisplay(display);
			return false;
		}
		
		if (ownDisplay)
			m_platformDisplay = display;
			
		Logger::info("VK: Xlib surface created successfully");
		return true;
	}
	else
	{
		Logger::error("VK: Unknown or invalid window type for Linux");
		return false;
	}
#else
	Logger::error("VK: Vulkan surface creation not implemented for this platform");
	return false;
#endif
}

bool VulkanDevice::selectPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		Logger::error("VK: No Vulkan devices found");
		return false;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(device, &props);

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

				if (presentSupport)
				{
					m_physicalDevice = device;
					m_graphicsQueueFamily = i;
					Logger::info("VK: Selected device: {}", props.deviceName);
					return true;
				}
			}
		}
	}

	Logger::error("VK: No suitable GPU found");
	return false;
}

bool VulkanDevice::createLogicalDevice()
{
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamily;
	queueCreateInfo.queueCount = 1;
	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures{};

	std::vector<const char*> extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create logical device");
		return false;
	}

	vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = m_physicalDevice;
	allocatorInfo.device = m_device;
	allocatorInfo.instance = m_instance;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

	if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create VMA allocator");
		return false;
	}

	Logger::info("VK: VMA allocator created successfully");
	return true;
}

uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	Logger::error("VK: Failed to find suitable memory type");
	return UINT32_MAX;
}
