// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "VulkanDevice.h"
#include "WindowInfo.h"

#if defined(_WIN32)
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#elif defined(__APPLE__)
#include <vulkan/vulkan_metal.h>
#include "CocoaTools.h"
#elif defined(__linux__)
#include "../Renderer.h"
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#include <wayland-client.h>
#include <vulkan/vulkan_wayland.h>
#endif

#include "Logger.h"

#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT /*type*/, const VkDebugUtilsMessengerCallbackDataEXT* data, void* /*ud*/)
{
	if (data && data->pMessage)
	{
		if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			Logger::error("VK validation: {}", data->pMessage);
		else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			Logger::warn("VK validation: {}", data->pMessage);
	}
	return VK_FALSE;
}
#endif

VulkanDevice::VulkanDevice() = default;

VulkanDevice::~VulkanDevice()
{
	destroy();
}

bool VulkanDevice::create(const WindowInfo& windowInfo, const std::string& preferredAdapter)
{
	m_error.Clear();
	if (!createInstance())
		return false;
	if (!createSurface(windowInfo))
	{
		destroy();
		return false;
	}
	if (!selectPhysicalDevice(preferredAdapter))
	{
		destroy();
		return false;
	}
	if (!createLogicalDevice())
	{
		destroy();
		return false;
	}
	return true;
}

std::vector<std::string> VulkanDevice::getAvailableAdapters()
{
	std::vector<std::string> adapterNames;

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "myMCpp";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "myMCpp";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	VkInstance tempInstance = VK_NULL_HANDLE;
	if (vkCreateInstance(&createInfo, nullptr, &tempInstance) == VK_SUCCESS)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(tempInstance, &deviceCount, nullptr);
		if (deviceCount > 0)
		{
			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(tempInstance, &deviceCount, devices.data());
			for (const auto& device : devices)
			{
				VkPhysicalDeviceProperties props;
				vkGetPhysicalDeviceProperties(device, &props);
				adapterNames.push_back(props.deviceName);
			}
		}
		vkDestroyInstance(tempInstance, nullptr);
	}

	return adapterNames;
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
#ifndef NDEBUG
		if (m_debugMessenger != VK_NULL_HANDLE)
		{
			auto destroyFn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
			if (destroyFn)
				destroyFn(m_instance, m_debugMessenger, nullptr);
			m_debugMessenger = VK_NULL_HANDLE;
		}
#endif
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
	appInfo.apiVersion = VK_API_VERSION_1_1;

	std::vector<const char*> extensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(__APPLE__)
		VK_EXT_METAL_SURFACE_EXTENSION_NAME,
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

#ifndef NDEBUG
	bool enableDebugValidation = false;
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	if (extensionCount > 0)
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

	const bool hasDebugUtilsExtension = std::any_of(availableExtensions.begin(), availableExtensions.end(),
		[](const VkExtensionProperties& extension) {
			return std::strcmp(extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
		});

	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	if (layerCount > 0)
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	const bool hasValidationLayer = std::any_of(availableLayers.begin(), availableLayers.end(),
		[](const VkLayerProperties& layer) {
			return std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0;
		});

	if (hasValidationLayer && hasDebugUtilsExtension)
	{
		enableDebugValidation = true;
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = layers;

		VkDebugUtilsMessengerCreateInfoEXT dbgInfo{};
		dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		dbgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		dbgInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		dbgInfo.pfnUserCallback = vulkanDebugCallback;
		createInfo.pNext = &dbgInfo;
	}
	else if (!hasValidationLayer)
	{
		Logger::warn("VK: Validation layer VK_LAYER_KHRONOS_validation not available, continuing without validation");
	}
	else
	{
		Logger::warn("VK: Extension VK_EXT_debug_utils not available, continuing without validation");
	}
#endif

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
		return m_error.Fail("VK: Failed to create Vulkan instance");

#ifndef NDEBUG
	if (enableDebugValidation)
	{
		auto createFn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
		if (createFn)
		{
			VkDebugUtilsMessengerCreateInfoEXT dbgCreate{};
			dbgCreate.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			dbgCreate.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			dbgCreate.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			dbgCreate.pfnUserCallback = vulkanDebugCallback;
			createFn(m_instance, &dbgCreate, nullptr, &m_debugMessenger);
		}
	}
#endif

	return true;
}

bool VulkanDevice::createSurface(const WindowInfo& windowInfo)
{
	if (windowInfo.type != WindowInfo::Type::Surfaceless && !windowInfo.window_handle)
		return m_error.Fail("VK: No native window handle available");

#if defined(_WIN32)
	if (windowInfo.type != WindowInfo::Type::Win32)
		return m_error.Fail("VK: Invalid window type for Windows");

	HWND hwnd = reinterpret_cast<HWND>(windowInfo.window_handle);

	if (!IsWindow(hwnd))
		return m_error.Fail("VK: Window handle is invalid");

	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = hwnd;
	createInfo.hinstance = GetModuleHandle(nullptr);

	VkResult result = vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);

	if (result != VK_SUCCESS)
		return m_error.Fail("VK: Failed to create window surface (VkResult " + std::to_string(static_cast<int>(result)) + ")");

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
			return m_error.Fail("VK: Failed to create Wayland surface");
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
				return m_error.Fail("VK: Failed to open X display");
			ownDisplay = true;
		}

		VkXlibSurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
		createInfo.dpy = display;
		createInfo.window = reinterpret_cast<Window>(windowInfo.window_handle);

		if (vkCreateXlibSurfaceKHR(m_instance, &createInfo, nullptr, &m_surface) != VK_SUCCESS)
		{
			if (ownDisplay)
				XCloseDisplay(display);
			return m_error.Fail("VK: Failed to create Xlib surface");
		}

		if (ownDisplay)
			m_platformDisplay = display;

		Logger::info("VK: Xlib surface created successfully");
		return true;
	}
	else
		return m_error.Fail("VK: Unknown or invalid window type for Linux");
#elif defined(__APPLE__)
	if (windowInfo.type != WindowInfo::Type::MacOS)
		return m_error.Fail("VK: Invalid window type for macOS");

	if (!windowInfo.surface_handle)
		return m_error.Fail("VK: Metal layer not created for MoltenVK");

	VkMetalSurfaceCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
	createInfo.pLayer = static_cast<const CAMetalLayer*>(windowInfo.surface_handle);

	VkResult result = vkCreateMetalSurfaceEXT(m_instance, &createInfo, nullptr, &m_surface);

	if (result != VK_SUCCESS)
		return m_error.Fail("VK: Failed to create Metal surface (VkResult " + std::to_string(static_cast<int>(result)) + ")");

	Logger::info("VK: Metal surface created successfully for MoltenVK");
	return true;
#else
	return m_error.Fail("VK: Vulkan surface creation not implemented for this platform");
#endif
}

bool VulkanDevice::selectPhysicalDevice(const std::string& preferredAdapter)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		return m_error.Fail("VK: No Vulkan devices found");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
	uint32_t selectedQueueFamily = 0;
	std::string selectedDeviceName;

	auto isDeviceSuitable = [&](VkPhysicalDevice device, uint32_t& queueFamilyIndex) {
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties2> queueFamilies(queueFamilyCount);
		for (uint32_t q = 0; q < queueFamilyCount; ++q)
		{
			queueFamilies[q].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
			queueFamilies[q].pNext = nullptr;
		}
		vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			if (queueFamilies[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				VkBool32 presentSupport = false;
				if (vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport) == VK_SUCCESS && presentSupport)
				{
					queueFamilyIndex = i;
					return true;
				}
			}
		}
		return false;
	};

	for (const auto& device : devices)
	{
		VkPhysicalDeviceProperties2 props2{};
		props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		vkGetPhysicalDeviceProperties2(device, &props2);

		uint32_t queueFamily = 0;
		if (isDeviceSuitable(device, queueFamily))
		{
			std::string name = props2.properties.deviceName;
			if (!preferredAdapter.empty() && name == preferredAdapter)
			{
				selectedDevice = device;
				selectedQueueFamily = queueFamily;
				selectedDeviceName = name;
				break;
			}

			if (selectedDevice == VK_NULL_HANDLE)
			{
				selectedDevice = device;
				selectedQueueFamily = queueFamily;
				selectedDeviceName = name;
			}
		}
	}

	if (selectedDevice != VK_NULL_HANDLE)
	{
		m_physicalDevice = selectedDevice;
		m_graphicsQueueFamily = selectedQueueFamily;
		Logger::info("VK: Using device: {}", selectedDeviceName);
		return true;
	}

	return m_error.Fail("VK: No suitable device found");
}

bool VulkanDevice::createLogicalDevice()
{
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamily;
	queueCreateInfo.queueCount = 1;
	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	if (extensionCount > 0)
		vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	auto hasExtension = [&](const char* name) {
		for (const auto& ext : availableExtensions)
		{
			if (std::strcmp(ext.extensionName, name) == 0)
				return true;
		}
		return false;
	};

	VkPhysicalDeviceMemoryPriorityFeaturesEXT memPriorityFeatures{};
	memPriorityFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT;

	VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageableFeatures{};
	pageableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT;

	VkPhysicalDeviceFeatures2 features2{};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

	void** chain = &features2.pNext;
	if (hasExtension(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
	{
		*chain = &memPriorityFeatures;
		chain = &memPriorityFeatures.pNext;
	}
	if (hasExtension(VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME))
		*chain = &pageableFeatures;

	vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);

	const bool supportsMemoryPriority = hasExtension(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME) && memPriorityFeatures.memoryPriority == VK_TRUE;
	const bool supportsPageable = hasExtension(VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME) && pageableFeatures.pageableDeviceLocalMemory == VK_TRUE && supportsMemoryPriority;

	std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	if (supportsMemoryPriority)
	{
		memPriorityFeatures.memoryPriority = VK_TRUE;
		extensions.push_back(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);
	}
	if (supportsPageable)
	{
		pageableFeatures.pageableDeviceLocalMemory = VK_TRUE;
		extensions.push_back(VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME);
	}

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &features2;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
		return m_error.Fail("VK: Failed to create logical device");

	m_supportsMemoryPriority = supportsMemoryPriority;

	vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = m_physicalDevice;
	allocatorInfo.device = m_device;
	allocatorInfo.instance = m_instance;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
	if (m_supportsMemoryPriority)
		allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;

	if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS)
		return m_error.Fail("VK: Failed to create VMA allocator");

	Logger::info("VK: VMA allocator created successfully");
	return true;
}

void VulkanDevice::setAllocationPriority(VmaAllocationCreateInfo& allocInfo, float priority) const
{
	if (m_supportsMemoryPriority)
		allocInfo.priority = priority;
}
