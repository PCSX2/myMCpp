// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "VulkanRenderer.h"
#include "ps2icon.h"
#include "ps2iconsys.h"
#include "Logger.h"
#include "../../../common/Config.h"
#include <vk_mem_alloc.h>
#include <cstring>
#include <algorithm>
#include <glm/glm.hpp>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#ifdef __APPLE__
#include "CocoaTools.h"
#endif

VulkanRenderer::VulkanRenderer(const WindowInfo& windowInfo, Config* config)
	: m_windowInfo(windowInfo)
	, m_width(windowInfo.surface_width)
	, m_height(windowInfo.surface_height)
	, m_initialized(false)
	, m_vertexBuffer(VK_NULL_HANDLE)
	, m_uniformBuffer(VK_NULL_HANDLE)
	, m_textureImage(VK_NULL_HANDLE)
	, m_textureView(VK_NULL_HANDLE)
	, m_textureSampler(VK_NULL_HANDLE)
	, m_descriptorPool(VK_NULL_HANDLE)
	, m_descriptorSet(VK_NULL_HANDLE)
	, m_iconChanged(false)
	, m_animationEnabled(false)
	, m_animStart(std::chrono::steady_clock::now())
	, m_vertexCount(0)
	, m_config(config)
{
	m_camera.applyMode();
	m_lighting.applyMode();
	RendererFactory::registerRenderer(this);
}

VulkanRenderer::~VulkanRenderer()
{
	RendererFactory::unregisterRenderer(this);
	shutdown();
}

bool VulkanRenderer::initialize()
{
	if (m_width < 100 || m_height < 100)
	{
		Logger::warn("VK: Window too small: {}x{}", m_width, m_height);
		m_width = std::max(m_width, 100u);
		m_height = std::max(m_height, 100u);
	}

#ifdef __APPLE__
	if (!m_windowInfo.surface_handle)
	{
		if (!CocoaTools::CreateMetalLayer(&m_windowInfo))
			return m_error.Fail("VK: Failed to create Metal layer for MoltenVK");
	}
	CocoaTools::SetDrawableSize(&m_windowInfo, m_width, m_height);
#endif

	if (!m_vulkanDevice.create(m_windowInfo, m_config ? m_config->getAdapter() : ""))
		return m_error.Assign(m_vulkanDevice.GetError());

	if (!m_vulkanSwapchain.create(m_vulkanDevice, m_width, m_height, m_config ? m_config->getVSync() : true))
		return m_error.Assign(m_vulkanSwapchain.GetError());

	VkRenderPass renderPass = m_vulkanSwapchain.getRenderPass();

	if (!m_vulkanPipeline.createPipelineCache(m_vulkanDevice.getDevice()))
		return m_error.Assign(m_vulkanPipeline.GetError());

	if (!m_vulkanPipeline.createMainPipeline(m_vulkanDevice.getDevice(), renderPass))
		return m_error.Assign(m_vulkanPipeline.GetError());

	if (!m_vulkanPipeline.createBackgroundPipeline(m_vulkanDevice.getDevice(), renderPass))
		return m_error.Assign(m_vulkanPipeline.GetError());

	if (!createDescriptorResources())
		return false;

	if (!m_vulkanResources.createCommandPool(m_vulkanDevice.getDevice(), m_vulkanDevice.getGraphicsQueueFamily()))
		return m_error.Assign(m_vulkanResources.GetError());

	if (!m_vulkanResources.createBackgroundVertexBuffer(m_vulkanDevice))
		return m_error.Assign(m_vulkanResources.GetError());

	if (!m_vulkanResources.createSyncObjects(m_vulkanDevice.getDevice(),
			static_cast<uint32_t>(m_vulkanSwapchain.getImages().size())))
		return m_error.Assign(m_vulkanResources.GetError());

	if (!allocateCommandBuffers())
		return false;

	m_imagesInFlight.resize(m_vulkanSwapchain.getImages().size(), VK_NULL_HANDLE);

	m_initialized = true;
	Logger::info("VK: Renderer initialized successfully");
	return true;
}

void VulkanRenderer::shutdown()
{
	VkDevice device = m_vulkanDevice.getDevice();
	if (device != VK_NULL_HANDLE)
	{
		VmaAllocator allocator = m_vulkanDevice.getAllocator();
		vkDeviceWaitIdle(device);

		if (!m_commandBuffers.empty())
		{
			vkFreeCommandBuffers(device, m_vulkanResources.getCommandPool(),
				static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
			m_commandBuffers.clear();
		}

		m_vulkanResources.destroy(device, allocator);
		m_vulkanPipeline.destroy(device);
		m_vulkanSwapchain.destroy(device, allocator);

		if (m_vertexBuffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(allocator, m_vertexBuffer, m_vertexAllocation);
			m_vertexBuffer = VK_NULL_HANDLE;
		}
		if (m_uniformBuffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(allocator, m_uniformBuffer, m_uniformAllocation);
			m_uniformBuffer = VK_NULL_HANDLE;
		}
		if (m_textureSampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(device, m_textureSampler, nullptr);
			m_textureSampler = VK_NULL_HANDLE;
		}
		if (m_textureView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(device, m_textureView, nullptr);
			m_textureView = VK_NULL_HANDLE;
		}
		if (m_textureImage != VK_NULL_HANDLE)
		{
			vmaDestroyImage(allocator, m_textureImage, m_textureAllocation);
			m_textureImage = VK_NULL_HANDLE;
		}
		if (m_stagingBuffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(allocator, m_stagingBuffer, m_stagingAllocation);
			m_stagingBuffer = VK_NULL_HANDLE;
		}
		if (m_descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
			m_descriptorPool = VK_NULL_HANDLE;
		}
	}

	m_vulkanDevice.destroy();
	m_initialized = false;
}

void VulkanRenderer::setIcon(std::shared_ptr<PS2Icon::Icon> icon)
{
	if (m_icon == icon)
		return;

	m_icon = icon;
	m_animStart = std::chrono::steady_clock::now();
	m_iconChanged = true;
}

bool VulkanRenderer::hasValidIcon() const
{
	return m_icon && m_icon->isValid();
}

void VulkanRenderer::setAnimationEnabled(bool enabled)
{
	m_animationEnabled = enabled;
	if (!enabled)
		prepareVertexData();
}

void VulkanRenderer::setRotation(float x, float y, float z)
{
	m_camera.setRotation(x, y, z);
}

void VulkanRenderer::setZoom(float zoom)
{
	m_camera.setZoom(zoom);
}

void VulkanRenderer::setCameraOffset(float x, float y, float z)
{
	m_camera.setOffset(x, y, z);
}

void VulkanRenderer::resetCamera()
{
	m_camera.reset();
}

void VulkanRenderer::setCameraMode(CameraMode mode)
{
	m_camera.mode = mode;
	m_camera.applyMode();
}

void VulkanRenderer::setLightingFromIconSys(PS2IconSys* iconSys)
{
	m_lighting.loadFromIconSys(iconSys);
}

void VulkanRenderer::setLightingMode(LightingMode mode)
{
	m_lighting.mode = mode;
	m_lighting.applyMode();
}

void VulkanRenderer::setBackgroundFromIconSys(PS2IconSys* iconSys)
{
	if (m_background.loadFromIconSys(iconSys))
		updateBackgroundVertexData();
}

void VulkanRenderer::setBackgroundColor(float r, float g, float b, float a)
{
	m_background.setColor(r, g, b, a);
	updateBackgroundVertexData();
}

void VulkanRenderer::render()
{
	if (!m_initialized || !hasValidIcon())
	{
		if (!m_initialized)
			Logger::error("VK: Not initialized");
		if (!hasValidIcon())
			Logger::error("VK: No valid icon");
		return;
	}

	VkDevice device = m_vulkanDevice.getDevice();

	if (m_iconChanged)
	{
		vkDeviceWaitIdle(device);
		prepareVertexData();
		uploadTexture();
		m_iconChanged = false;
		Logger::info("VK: Icon loaded successfully");
	}

	if (m_animationEnabled && m_icon && m_icon->getAnimationShapes() > 1 && m_icon->getFrameCount() > 0 && m_vertexBuffer != VK_NULL_HANDLE)
	{
		double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_animStart).count();
		float duration = static_cast<float>(m_icon->getFrameLength());
		float animTime = AnimationUtils::computeAnimationTime(elapsed, duration, m_icon->getAnimSpeed());

		auto shapeWeights = AnimationUtils::computeShapeWeights(m_icon.get(), animTime, duration);
		auto blendedVerts = AnimationUtils::blendVertices(m_icon.get(), shapeWeights, m_vertexCount);

		std::vector<VulkanVertex> vertices(m_vertexCount);
		for (uint32_t i = 0; i < m_vertexCount; ++i)
		{
			vertices[i].pos[0] = blendedVerts[i].pos[0];
			vertices[i].pos[1] = blendedVerts[i].pos[1];
			vertices[i].pos[2] = blendedVerts[i].pos[2];
			vertices[i].normal[0] = blendedVerts[i].normal[0];
			vertices[i].normal[1] = blendedVerts[i].normal[1];
			vertices[i].normal[2] = blendedVerts[i].normal[2];
			vertices[i].texCoord[0] = blendedVerts[i].texCoord[0];
			vertices[i].texCoord[1] = blendedVerts[i].texCoord[1];
			vertices[i].color[0] = blendedVerts[i].color[0];
			vertices[i].color[1] = blendedVerts[i].color[1];
			vertices[i].color[2] = blendedVerts[i].color[2];
			vertices[i].color[3] = blendedVerts[i].color[3];
		}

		VkDeviceSize size = vertices.size() * sizeof(VulkanVertex);
		std::memcpy(m_vertexAllocInfo.pMappedData, vertices.data(), static_cast<size_t>(size));
	}

	updateUniformBuffer();
	submitFrame();
}

void VulkanRenderer::resize(uint32_t width, uint32_t height)
{
	if (!m_initialized)
		return;

#ifdef __APPLE__
	CocoaTools::SetDrawableSize(&m_windowInfo, width, height);
#endif

	m_width = width;
	m_height = height;
}

bool VulkanRenderer::swapchainNeedsRecreate() const
{
	if (m_width == 0 || m_height == 0)
		return false;

	VkExtent2D extent = m_vulkanSwapchain.getExtent();
	return m_width != extent.width || m_height != extent.height;
}

bool VulkanRenderer::recreateSwapchain()
{
	VkDevice device = m_vulkanDevice.getDevice();

	for (VkFence fence : m_imagesInFlight)
	{
		if (fence != VK_NULL_HANDLE)
			vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	}

	for (uint32_t i = 0; i < VulkanResources::frameCount; ++i)
	{
		VkFence fence = m_vulkanResources.getInFlightFence(i);
		if (fence != VK_NULL_HANDLE)
			vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	}

	if (!m_commandBuffers.empty())
	{
		vkFreeCommandBuffers(device, m_vulkanResources.getCommandPool(),
			static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
		m_commandBuffers.clear();
	}

	VkExtent2D extent = m_vulkanSwapchain.getExtent();
	if (m_width != extent.width || m_height != extent.height)
		Logger::info("VK: Resizing to {}x{}", m_width, m_height);

	if (!m_vulkanSwapchain.recreate(m_vulkanDevice, m_width, m_height))
	{
		Logger::error("VK: Failed to recreate swapchain");
		return false;
	}

	if (!rebuildPerImageResources())
	{
		Logger::error("VK: Failed to rebuild per-image resources after resize");
		return false;
	}

	VkExtent2D actualExtent = m_vulkanSwapchain.getExtent();
	m_width = actualExtent.width;
	m_height = actualExtent.height;

	return true;
}

bool VulkanRenderer::rebuildPerImageResources()
{
	VkDevice device = m_vulkanDevice.getDevice();

	m_imagesInFlight.clear();
	m_imagesInFlight.resize(m_vulkanSwapchain.getImages().size(), VK_NULL_HANDLE);

	if (!m_vulkanResources.recreateRenderFinishedSemaphores(device,
			static_cast<uint32_t>(m_vulkanSwapchain.getImages().size())))
		return false;

	return allocateCommandBuffers();
}

uint32_t VulkanRenderer::getVertexCount() const
{
	return m_vertexCount;
}

uint32_t VulkanRenderer::getFrameCount() const
{
	if (!m_icon)
		return 0;
	return m_icon->getFrameCount();
}

void VulkanRenderer::prepareVertexData()
{
	if (!m_icon || !m_icon->isValid())
		return;

	m_vertexCount = m_icon->getVertexCount();

	std::vector<VulkanVertex> vertices;
	vertices.reserve(m_vertexCount);

	const auto* vertexCoords = m_icon->getVertexData();
	const auto* normalUV = m_icon->getNormalUVData();
	const auto* colors = m_icon->getColorData();

	for (uint32_t i = 0; i < m_vertexCount; ++i)
	{
		VulkanVertex v{};
		v.pos[0] = vertexCoords[i].x;
		v.pos[1] = vertexCoords[i].y;
		v.pos[2] = vertexCoords[i].z;
		v.normal[0] = normalUV[i].nx;
		v.normal[1] = normalUV[i].ny;
		v.normal[2] = normalUV[i].nz;
		v.texCoord[0] = normalUV[i].u;
		v.texCoord[1] = normalUV[i].v;
		v.color[0] = colors[i].r;
		v.color[1] = colors[i].g;
		v.color[2] = colors[i].b;
		v.color[3] = colors[i].a;
		vertices.push_back(v);
	}

	const VkDeviceSize requiredSize = vertices.size() * sizeof(VulkanVertex);

	if (m_vertexBuffer != VK_NULL_HANDLE && m_vertexBufferSize >= requiredSize)
	{
		std::memcpy(m_vertexAllocInfo.pMappedData, vertices.data(), static_cast<size_t>(requiredSize));
	}
	else
	{
		VmaAllocator allocator = m_vulkanDevice.getAllocator();
		if (m_vertexBuffer != VK_NULL_HANDLE)
			vmaDestroyBuffer(allocator, m_vertexBuffer, m_vertexAllocation);
		m_vertexBuffer = VK_NULL_HANDLE;
		m_vertexAllocation = VK_NULL_HANDLE;
		m_vertexBufferSize = 0;

		if (!m_vulkanResources.createMappedBuffer(m_vulkanDevice, requiredSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0.5f,
				m_vertexBuffer, m_vertexAllocation, m_vertexAllocInfo))
		{
			Logger::error("{}", m_vulkanResources.GetError().GetDescription());
			return;
		}

		std::memcpy(m_vertexAllocInfo.pMappedData, vertices.data(), static_cast<size_t>(requiredSize));
		m_vertexBufferSize = requiredSize;
	}

	Logger::info("VK: Prepared {} vertices", m_vertexCount);
}

void VulkanRenderer::writeDescriptorSets()
{
	if (m_descriptorSet == VK_NULL_HANDLE || m_uniformBuffer == VK_NULL_HANDLE || m_textureView == VK_NULL_HANDLE)
		return;

	VkDevice device = m_vulkanDevice.getDevice();
	const VkDeviceSize uboSize = 320;

	VkDescriptorBufferInfo bufferDescInfo{};
	bufferDescInfo.buffer = m_uniformBuffer;
	bufferDescInfo.offset = 0;
	bufferDescInfo.range = uboSize;

	VkDescriptorImageInfo imageDescInfo{};
	imageDescInfo.sampler = m_textureSampler;
	imageDescInfo.imageView = m_textureView;
	imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet descriptorWrites[2]{};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = m_descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferDescInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = m_descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageDescInfo;

	vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
}

bool VulkanRenderer::createDescriptorResources()
{
	VkDevice device = m_vulkanDevice.getDevice();
	const VkDeviceSize uboSize = 320;

	if (!m_vulkanResources.createMappedBuffer(m_vulkanDevice, uboSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 0.5f,
			m_uniformBuffer, m_uniformAllocation, m_uniformAllocInfo))
		return m_error.Assign(m_vulkanResources.GetError());

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
		return m_error.Fail("VK: Failed to create texture sampler");

	if (!m_vulkanResources.createImage(m_vulkanDevice, 1, 1, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0.5f,
			m_textureImage, m_textureAllocation))
		return m_error.Assign(m_vulkanResources.GetError());

	m_textureWidth = 1;
	m_textureHeight = 1;

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device, &viewInfo, nullptr, &m_textureView) != VK_SUCCESS)
		return m_error.Fail("VK: Failed to create placeholder texture view");

	VkDescriptorPoolSize poolSizes[2]{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		return m_error.Fail("VK: Failed to create descriptor pool");

	VkDescriptorSetLayout layout = m_vulkanPipeline.getDescriptorSetLayout();
	VkDescriptorSetAllocateInfo allocInfoDesc{};
	allocInfoDesc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfoDesc.descriptorPool = m_descriptorPool;
	allocInfoDesc.descriptorSetCount = 1;
	allocInfoDesc.pSetLayouts = &layout;

	if (vkAllocateDescriptorSets(device, &allocInfoDesc, &m_descriptorSet) != VK_SUCCESS)
		return m_error.Fail("VK: Failed to allocate descriptor set");

	writeDescriptorSets();
	return true;
}

void VulkanRenderer::updateUniformBuffer()
{
	VkExtent2D extent = m_vulkanSwapchain.getExtent();
	float autoRotateY = 0.0f;
	if (m_animationEnabled)
	{
		double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_animStart).count();
		autoRotateY = static_cast<float>(elapsed * -0.5);
	}
	auto matrices = CameraUtils::computeMatrices(m_camera, extent.width, extent.height, autoRotateY);
	auto uboData = UniformBufferUtils::buildUniformBufferData(matrices, m_lighting, true);
	std::memcpy(m_uniformAllocInfo.pMappedData, &uboData, sizeof(uboData));
}

bool VulkanRenderer::uploadTexture()
{
	if (!m_icon)
		return false;

	VkDevice device = m_vulkanDevice.getDevice();
	VkQueue queue = m_vulkanDevice.getGraphicsQueue();
	VmaAllocator allocator = m_vulkanDevice.getAllocator();

	const auto* textureData = m_icon->getTextureData();
	if (!textureData)
	{
		Logger::warn("VK: Icon has no texture data");
		return false;
	}

	uint32_t texWidth = m_icon->getTextureWidth();
	uint32_t texHeight = m_icon->getTextureHeight();
	const VkFormat textureFormat = VK_FORMAT_R8G8B8A8_SRGB;

	if (texWidth == 0 || texHeight == 0)
	{
		Logger::warn("VK: Invalid icon texture size");
		return false;
	}

	auto rgba = TextureUtils::convertPS2TextureToRGBA(textureData, texWidth, texHeight);
	if (rgba.empty())
	{
		Logger::warn("VK: Texture conversion returned empty data");
		return false;
	}

	VkDeviceSize imageSize = rgba.size();

	if (m_stagingBuffer == VK_NULL_HANDLE || m_stagingBufferSize < imageSize)
	{
		if (m_stagingBuffer != VK_NULL_HANDLE)
			vmaDestroyBuffer(allocator, m_stagingBuffer, m_stagingAllocation);

		if (!m_vulkanResources.createMappedBuffer(m_vulkanDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0.0f,
				m_stagingBuffer, m_stagingAllocation, m_stagingAllocInfo))
		{
			Logger::error("{}", m_vulkanResources.GetError().GetDescription());
			m_stagingBuffer = VK_NULL_HANDLE;
			return false;
		}

		m_stagingBufferSize = imageSize;
	}

	std::memcpy(m_stagingAllocInfo.pMappedData, rgba.data(), static_cast<size_t>(imageSize));

	const bool newTexture = m_textureImage == VK_NULL_HANDLE || m_textureWidth != texWidth || m_textureHeight != texHeight;

	if (newTexture)
	{
		if (m_textureView != VK_NULL_HANDLE)
			vkDestroyImageView(device, m_textureView, nullptr);
		if (m_textureImage != VK_NULL_HANDLE)
			vmaDestroyImage(allocator, m_textureImage, m_textureAllocation);
		m_textureView = VK_NULL_HANDLE;

		if (!m_vulkanResources.createImage(m_vulkanDevice, texWidth, texHeight, textureFormat,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0.5f,
				m_textureImage, m_textureAllocation))
		{
			Logger::error("{}", m_vulkanResources.GetError().GetDescription());
			return false;
		}
		m_textureWidth = texWidth;
		m_textureHeight = texHeight;

		if (!m_vulkanResources.transitionImageLayout(device, queue, m_textureImage, textureFormat,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
			return false;

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_textureImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = textureFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &viewInfo, nullptr, &m_textureView) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to create texture image view");
			return false;
		}
	}
	else if (!m_vulkanResources.transitionImageLayout(device, queue, m_textureImage, textureFormat,
				 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
	{
		return false;
	}

	if (!m_vulkanResources.copyBufferToImage(device, queue, m_stagingBuffer, m_textureImage, texWidth, texHeight))
		return false;
	if (!m_vulkanResources.transitionImageLayout(device, queue, m_textureImage, textureFormat,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
		return false;

	if (newTexture)
	{
		Logger::info("VK: Texture loaded: {}x{}", texWidth, texHeight);
		writeDescriptorSets();
	}
	updateUniformBuffer();

	return true;
}

void VulkanRenderer::updateBackgroundVertexData()
{
	VulkanBGVertex verts[4];
	auto writeColor = [](const glm::vec4& c) {
		VulkanBGVertex v{};
		v.color[0] = static_cast<uint8_t>(glm::clamp(c.r, 0.0f, 1.0f) * 255.0f);
		v.color[1] = static_cast<uint8_t>(glm::clamp(c.g, 0.0f, 1.0f) * 255.0f);
		v.color[2] = static_cast<uint8_t>(glm::clamp(c.b, 0.0f, 1.0f) * 255.0f);
		v.color[3] = static_cast<uint8_t>(glm::clamp(c.a, 0.0f, 1.0f) * 255.0f);
		return v;
	};

	verts[0] = writeColor(m_background.colors[0]);
	verts[0].pos[0] = -1.0f;
	verts[0].pos[1] = 1.0f;
	verts[1] = writeColor(m_background.colors[2]);
	verts[1].pos[0] = -1.0f;
	verts[1].pos[1] = -1.0f;
	verts[2] = writeColor(m_background.colors[1]);
	verts[2].pos[0] = 1.0f;
	verts[2].pos[1] = 1.0f;
	verts[3] = writeColor(m_background.colors[3]);
	verts[3].pos[0] = 1.0f;
	verts[3].pos[1] = -1.0f;

	m_vulkanResources.updateBackgroundVertexData(verts, 4);
}

bool VulkanRenderer::allocateCommandBuffers()
{
	VkDevice device = m_vulkanDevice.getDevice();

	if (!m_commandBuffers.empty())
	{
		vkFreeCommandBuffers(device, m_vulkanResources.getCommandPool(),
			static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
		m_commandBuffers.clear();
	}

	const auto& swapchainImages = m_vulkanSwapchain.getImages();

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_vulkanResources.getCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(swapchainImages.size());

	m_commandBuffers.resize(swapchainImages.size());

	if (vkAllocateCommandBuffers(device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
	{
		m_commandBuffers.clear();
		return m_error.Fail("VK: Failed to allocate command buffers");
	}

	return true;
}

bool VulkanRenderer::recordCommandBuffer(uint32_t imageIndex)
{
	const auto& framebuffers = m_vulkanSwapchain.getFramebuffers();
	VkExtent2D extent = m_vulkanSwapchain.getExtent();
	VkRenderPass renderPass = m_vulkanSwapchain.getRenderPass();
	VkCommandBuffer cmd = m_commandBuffers[imageIndex];

	vkResetCommandBuffer(cmd, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to record command buffer");
		return false;
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = extent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clearValues[1].depthStencil = {1.0f, 0};
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = extent;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkBuffer bgBuffer = m_vulkanResources.getBackgroundVertexBuffer();
	if (m_background.shouldRender && m_vulkanPipeline.getBackgroundPipeline() != VK_NULL_HANDLE && bgBuffer != VK_NULL_HANDLE)
	{
		VkBuffer bgBuffers[] = {bgBuffer};
		VkDeviceSize bgOffsets[] = {0};
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipeline.getBackgroundPipeline());
		vkCmdSetViewport(cmd, 0, 1, &viewport);
		vkCmdSetScissor(cmd, 0, 1, &scissor);
		vkCmdBindVertexBuffers(cmd, 0, 1, bgBuffers, bgOffsets);
		vkCmdDraw(cmd, 4, 1, 0, 0);
	}

	if (m_vertexBuffer != VK_NULL_HANDLE && m_vertexCount > 0)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipeline.getGraphicsPipeline());
		vkCmdSetViewport(cmd, 0, 1, &viewport);
		vkCmdSetScissor(cmd, 0, 1, &scissor);
		VkBuffer vertexBuffers[] = {m_vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

		if (m_descriptorSet != VK_NULL_HANDLE)
		{
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_vulkanPipeline.getPipelineLayout(), 0, 1, &m_descriptorSet, 0, nullptr);
		}

		struct PushConstants
		{
			int32_t useTexture;
			int32_t enableAlpha;
			float alphaOverride;
		} pc;
		pc.useTexture = (m_icon && m_icon->hasTexture()) ? 1 : 0;
		pc.enableAlpha = (m_icon && m_icon->hasAlpha()) ? 1 : 0;
		pc.alphaOverride = 1.0f;
		vkCmdPushConstants(cmd, m_vulkanPipeline.getPipelineLayout(),
			VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

		vkCmdDraw(cmd, m_vertexCount, 1, 0, 0);
	}

	vkCmdEndRenderPass(cmd);

	if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to record command buffer");
		return false;
	}

	return true;
}

void VulkanRenderer::submitFrame()
{
	VkDevice device = m_vulkanDevice.getDevice();
	VkQueue queue = m_vulkanDevice.getGraphicsQueue();
	VkSwapchainKHR swapchain = m_vulkanSwapchain.getSwapchain();

	if (!m_initialized || swapchain == VK_NULL_HANDLE)
	{
		Logger::error("VK: Not initialized or no swapchain");
		return;
	}

	if (swapchainNeedsRecreate())
	{
		recreateSwapchain();
		return;
	}

	VkFence fence = m_vulkanResources.getInFlightFence(m_currentFrame);
	VkSemaphore imageAvailable = m_vulkanResources.getImageAvailableSemaphore(m_currentFrame);
	vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapchain();
		return;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		Logger::error("VK: Failed to acquire next image: {}", static_cast<int>(result));
		return;
	}

	VkSemaphore renderFinished = m_vulkanResources.getRenderFinishedSemaphore(imageIndex);

	if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(device, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	m_imagesInFlight[imageIndex] = fence;

	vkResetFences(device, 1, &fence);

	if (!recordCommandBuffer(imageIndex))
		return;

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailable;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinished;

	if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to submit draw command buffer");
		return;
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinished;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(queue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		recreateSwapchain();
		return;
	}

	if (result != VK_SUCCESS)
	{
		Logger::error("VK: Failed to present image: {}", static_cast<int>(result));
	}

	m_currentFrame = (m_currentFrame + 1) % VulkanResources::frameCount;
}

void VulkanRenderer::setVSync(bool enabled)
{
	if (m_vulkanDevice.getDevice() == VK_NULL_HANDLE || !m_initialized)
		return;

	VkDevice device = m_vulkanDevice.getDevice();
	if (m_vulkanSwapchain.setVSync(m_vulkanDevice, enabled))
	{
		vkDeviceWaitIdle(device);

		if (!m_commandBuffers.empty())
		{
			vkFreeCommandBuffers(device, m_vulkanResources.getCommandPool(),
				static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
			m_commandBuffers.clear();
		}

		if (!rebuildPerImageResources())
			Logger::error("VK: Failed to rebuild per-image resources after VSync change");
	}

	if (m_config)
		m_config->setVSync(enabled);
}
