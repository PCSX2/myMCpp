// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "VulkanRenderer.h"
#include "ps2icon.h"
#include "ps2iconsys.h"
#include "../../../common/Logger.h"
#include <cstring>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include <cmath>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

VulkanRenderer::VulkanRenderer(const WindowInfo& windowInfo)
	: m_windowInfo(windowInfo)
	, m_width(windowInfo.surface_width)
	, m_height(windowInfo.surface_height)
	, m_initialized(false)
	, m_vertexBuffer(VK_NULL_HANDLE)
	, m_vertexMemory(VK_NULL_HANDLE)
	, m_uniformBuffer(VK_NULL_HANDLE)
	, m_uniformMemory(VK_NULL_HANDLE)
	, m_textureImage(VK_NULL_HANDLE)
	, m_textureMemory(VK_NULL_HANDLE)
	, m_textureView(VK_NULL_HANDLE)
	, m_textureSampler(VK_NULL_HANDLE)
	, m_descriptorPool(VK_NULL_HANDLE)
	, m_descriptorSet(VK_NULL_HANDLE)
	, m_iconChanged(false)
	, m_animationEnabled(false)
	, m_animStart(std::chrono::steady_clock::now())
	, m_vertexCount(0)
{
	m_camera.applyMode();
	m_lighting.applyMode();
}

VulkanRenderer::~VulkanRenderer()
{
	shutdown();
}

bool VulkanRenderer::initialize()
{
	Logger::info("VK: VulkanRenderer::initialize() - Size: {}x{} Ptr: {}", m_width, m_height, (void*)this);

	if (m_width < 100 || m_height < 100)
	{
		Logger::warn("VK: Window too small: {}x{}", m_width, m_height);
		m_width = std::max(m_width, 100u);
		m_height = std::max(m_height, 100u);
	}

	if (!m_vulkanDevice.create(m_windowInfo))
		return false;

	if (!m_vulkanSwapchain.create(m_vulkanDevice, m_width, m_height))
		return false;

	VkRenderPass renderPass = m_vulkanSwapchain.getRenderPass();
	VkExtent2D extent = m_vulkanSwapchain.getExtent();

	if (!m_vulkanPipeline.createMainPipeline(m_vulkanDevice.getDevice(), renderPass, extent))
		return false;

	if (!m_vulkanPipeline.createBackgroundPipeline(m_vulkanDevice.getDevice(), renderPass, extent))
		return false;

	if (!m_vulkanResources.createCommandPool(m_vulkanDevice.getDevice(), m_vulkanDevice.getGraphicsQueueFamily()))
		return false;

	if (!m_vulkanResources.createBackgroundVertexBuffer(m_vulkanDevice))
		return false;

	if (!m_vulkanResources.createSyncObjects(m_vulkanDevice.getDevice()))
		return false;

	if (!createCommandBuffers())
		return false;

	m_initialized = true;
	Logger::info("VK: VulkanRenderer initialized successfully");
	return true;
}

void VulkanRenderer::shutdown()
{
	if (!m_initialized)
		return;

	VkDevice device = m_vulkanDevice.getDevice();
	if (device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(device);

		if (!m_commandBuffers.empty())
		{
			vkFreeCommandBuffers(device, m_vulkanResources.getCommandPool(),
				static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
			m_commandBuffers.clear();
		}

		if (m_vertexBuffer != VK_NULL_HANDLE)
			vkDestroyBuffer(device, m_vertexBuffer, nullptr);
		if (m_vertexMemory != VK_NULL_HANDLE)
			vkFreeMemory(device, m_vertexMemory, nullptr);
		if (m_uniformBuffer != VK_NULL_HANDLE)
			vkDestroyBuffer(device, m_uniformBuffer, nullptr);
		if (m_uniformMemory != VK_NULL_HANDLE)
			vkFreeMemory(device, m_uniformMemory, nullptr);
		if (m_textureSampler != VK_NULL_HANDLE)
			vkDestroySampler(device, m_textureSampler, nullptr);
		if (m_textureView != VK_NULL_HANDLE)
			vkDestroyImageView(device, m_textureView, nullptr);
		if (m_textureImage != VK_NULL_HANDLE)
			vkDestroyImage(device, m_textureImage, nullptr);
		if (m_textureMemory != VK_NULL_HANDLE)
			vkFreeMemory(device, m_textureMemory, nullptr);
		if (m_descriptorPool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);

		m_vulkanResources.destroy(device);
		m_vulkanPipeline.destroy(device);
		m_vulkanSwapchain.destroy(device);
	}
	m_initialized = false;
}

void VulkanRenderer::setIcon(std::shared_ptr<PS2Icon::Icon> icon)
{
	m_icon = icon;
	m_animStart = std::chrono::steady_clock::now();
	m_iconChanged = true;
	Logger::info("VK: Icon set");
}

bool VulkanRenderer::hasValidIcon() const
{
	return m_icon && m_icon->isValid();
}

void VulkanRenderer::setAnimationEnabled(bool enabled)
{
	m_animationEnabled = enabled;
	if (!enabled)
	{
		// Reset to base pose
		prepareVertexData();
		m_iconChanged = true;
	}
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
	{
		updateBackgroundVertexData();
		m_iconChanged = true;
	}
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
		Logger::debug("VK: Processing icon change");
		prepareVertexData();
		uploadTexture();
		createCommandBuffers();
		m_iconChanged = false;
		Logger::debug("VK: Icon prepared with {} vertices", m_vertexCount);
	}

	if (m_animationEnabled && m_icon && m_icon->getAnimationShapes() > 1 && m_icon->getFrameCount() > 0 && m_vertexBuffer != VK_NULL_HANDLE)
	{
		double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_animStart).count();
		float duration = static_cast<float>(m_icon->getFrameLength());
		float animTime = AnimationUtils::computeAnimationTime(elapsed, duration);

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
		void* mapped = nullptr;
		if (vkMapMemory(device, m_vertexMemory, 0, size, 0, &mapped) == VK_SUCCESS)
		{
			std::memcpy(mapped, vertices.data(), size);
			vkUnmapMemory(device, m_vertexMemory);
		}
	}

	updateUniformBuffer();
	Logger::debug("VK: Uniform buffer updated, submitting frame...");
	submitFrame();
}

void VulkanRenderer::resize(uint32_t width, uint32_t height)
{
	if (!m_initialized)
		return;

	m_width = width;
	m_height = height;
	Logger::info("VK: Resizing to {}x{}", width, height);

	VkDevice device = m_vulkanDevice.getDevice();
	vkDeviceWaitIdle(device);

	if (!m_commandBuffers.empty())
	{
		vkFreeCommandBuffers(device, m_vulkanResources.getCommandPool(),
			static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
		m_commandBuffers.clear();
	}

	m_vulkanPipeline.destroy(device);

	if (!m_vulkanSwapchain.recreate(m_vulkanDevice, width, height))
	{
		Logger::error("VK: Failed to recreate swapchain");
		return;
	}

	VkRenderPass renderPass = m_vulkanSwapchain.getRenderPass();
	VkExtent2D extent = m_vulkanSwapchain.getExtent();

	if (!m_vulkanPipeline.createMainPipeline(device, renderPass, extent))
	{
		Logger::error("VK: Failed to recreate main pipeline");
		return;
	}

	if (!m_vulkanPipeline.createBackgroundPipeline(device, renderPass, extent))
	{
		Logger::error("VK: Failed to recreate background pipeline");
		return;
	}

	if (!createCommandBuffers())
	{
		Logger::error("VK: Failed to recreate command buffers");
		return;
	}

	m_iconChanged = true;
	render();
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

	VkDevice device = m_vulkanDevice.getDevice();
	m_vertexCount = m_icon->getVertexCount();
	Logger::debug("VK: Preparing {} vertices", m_vertexCount);

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

	if (m_vertexBuffer != VK_NULL_HANDLE)
		vkDestroyBuffer(device, m_vertexBuffer, nullptr);
	if (m_vertexMemory != VK_NULL_HANDLE)
		vkFreeMemory(device, m_vertexMemory, nullptr);

	VkBufferCreateInfo vertexBufferInfo{};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.size = vertices.size() * sizeof(VulkanVertex);
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &vertexBufferInfo, nullptr, &m_vertexBuffer) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create vertex buffer");
		return;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, m_vertexBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = m_vulkanDevice.findMemoryType(memRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (allocInfo.memoryTypeIndex == UINT32_MAX)
	{
		Logger::error("VK: Failed to find memory type for vertex buffer");
		vkDestroyBuffer(device, m_vertexBuffer, nullptr);
		return;
	}

	if (vkAllocateMemory(device, &allocInfo, nullptr, &m_vertexMemory) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to allocate vertex memory");
		vkDestroyBuffer(device, m_vertexBuffer, nullptr);
		return;
	}

	vkBindBufferMemory(device, m_vertexBuffer, m_vertexMemory, 0);

	void* data;
	vkMapMemory(device, m_vertexMemory, 0, vertexBufferInfo.size, 0, &data);
	std::memcpy(data, vertices.data(), vertexBufferInfo.size);
	vkUnmapMemory(device, m_vertexMemory);

	Logger::debug("VK: Vertex buffer created (host-visible) for animation updates");
}

void VulkanRenderer::updateUniformBuffer()
{
	VkDevice device = m_vulkanDevice.getDevice();
	const VkDeviceSize uboSize = 320;

	if (m_uniformBuffer == VK_NULL_HANDLE)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = uboSize;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_uniformBuffer) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to create uniform buffer");
			return;
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, m_uniformBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = m_vulkanDevice.findMemoryType(memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (allocInfo.memoryTypeIndex == UINT32_MAX)
		{
			Logger::error("VK: Failed to find memory type for uniform buffer");
			vkDestroyBuffer(device, m_uniformBuffer, nullptr);
			m_uniformBuffer = VK_NULL_HANDLE;
			return;
		}

		if (vkAllocateMemory(device, &allocInfo, nullptr, &m_uniformMemory) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to allocate uniform memory");
			vkDestroyBuffer(device, m_uniformBuffer, nullptr);
			m_uniformBuffer = VK_NULL_HANDLE;
			return;
		}

		vkBindBufferMemory(device, m_uniformBuffer, m_uniformMemory, 0);

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

		if (vkCreateSampler(device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to create texture sampler");
		}

		if (m_textureView == VK_NULL_HANDLE)
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = 1;
			imageInfo.extent.height = 1;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

			if (vkCreateImage(device, &imageInfo, nullptr, &m_textureImage) == VK_SUCCESS)
			{
				VkMemoryRequirements imgMemReq;
				vkGetImageMemoryRequirements(device, m_textureImage, &imgMemReq);

				VkMemoryAllocateInfo imgAllocInfo{};
				imgAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				imgAllocInfo.allocationSize = imgMemReq.size;
				imgAllocInfo.memoryTypeIndex = m_vulkanDevice.findMemoryType(imgMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				if (imgAllocInfo.memoryTypeIndex != UINT32_MAX)
				{
					vkAllocateMemory(device, &imgAllocInfo, nullptr, &m_textureMemory);
					vkBindImageMemory(device, m_textureImage, m_textureMemory, 0);

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

					vkCreateImageView(device, &viewInfo, nullptr, &m_textureView);
				}
			}
		}

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
		{
			Logger::error("VK: Failed to create descriptor pool");
			return;
		}

		VkDescriptorSetLayout layout = m_vulkanPipeline.getDescriptorSetLayout();
		VkDescriptorSetAllocateInfo allocInfoDesc{};
		allocInfoDesc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfoDesc.descriptorPool = m_descriptorPool;
		allocInfoDesc.descriptorSetCount = 1;
		allocInfoDesc.pSetLayouts = &layout;

		if (vkAllocateDescriptorSets(device, &allocInfoDesc, &m_descriptorSet) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to allocate descriptor set");
			return;
		}

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
	else if (m_descriptorSet != VK_NULL_HANDLE && m_textureView != VK_NULL_HANDLE)
	{
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

	void* data;
	vkMapMemory(device, m_uniformMemory, 0, uboSize, 0, &data);

	VkExtent2D extent = m_vulkanSwapchain.getExtent();
	float autoRotateY = 0.0f;
	if (m_animationEnabled)
	{
		double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_animStart).count();
		autoRotateY = static_cast<float>(elapsed * -0.5);
	}
	auto matrices = CameraUtils::computeMatrices(m_camera, extent.width, extent.height, autoRotateY);
	auto uboData = UniformBufferUtils::buildUniformBufferData(matrices, m_lighting, true);
	std::memcpy(data, &uboData, sizeof(uboData));

	vkUnmapMemory(device, m_uniformMemory);
}

bool VulkanRenderer::uploadTexture()
{
	if (!m_icon)
		return false;

	VkDevice device = m_vulkanDevice.getDevice();
	VkQueue queue = m_vulkanDevice.getGraphicsQueue();

	const auto* textureData = m_icon->getTextureData();
	if (!textureData)
	{
		Logger::warn("VK: Icon has no texture data");
		return false;
	}

	uint32_t texWidth = m_icon->getTextureWidth();
	uint32_t texHeight = m_icon->getTextureHeight();
	const VkFormat textureFormat = VK_FORMAT_R8G8B8A8_UNORM;

	if (texWidth == 0 || texHeight == 0)
	{
		Logger::warn("VK: Invalid icon texture size");
		return false;
	}

	auto rgba = TextureUtils::convertPS2TextureToRGBA(textureData, texWidth, texHeight);
	VkDeviceSize imageSize = rgba.size();

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = imageSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create texture staging buffer");
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = m_vulkanDevice.findMemoryType(memRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (allocInfo.memoryTypeIndex == UINT32_MAX)
	{
		Logger::error("VK: Failed to find memory type for texture staging");
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		return false;
	}

	if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to allocate texture staging memory");
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		return false;
	}

	vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	std::memcpy(data, rgba.data(), static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	if (m_textureView != VK_NULL_HANDLE)
		vkDestroyImageView(device, m_textureView, nullptr);
	if (m_textureImage != VK_NULL_HANDLE)
		vkDestroyImage(device, m_textureImage, nullptr);
	if (m_textureMemory != VK_NULL_HANDLE)
		vkFreeMemory(device, m_textureMemory, nullptr);

	if (!m_vulkanResources.createImage(m_vulkanDevice, texWidth, texHeight, textureFormat, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_textureImage, m_textureMemory))
	{
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
		return false;
	}

	m_vulkanResources.transitionImageLayout(device, queue, m_textureImage, textureFormat,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_vulkanResources.copyBufferToImage(device, queue, stagingBuffer, m_textureImage, texWidth, texHeight);
	m_vulkanResources.transitionImageLayout(device, queue, m_textureImage, textureFormat,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
		return false;
	}

	updateUniformBuffer();

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	return true;
}

void VulkanRenderer::updateBackgroundVertexData()
{
	VkDevice device = m_vulkanDevice.getDevice();

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

	m_vulkanResources.updateBackgroundVertexData(device, verts, 4);
}

bool VulkanRenderer::createCommandBuffers()
{
	VkDevice device = m_vulkanDevice.getDevice();

	if (!m_commandBuffers.empty())
	{
		vkFreeCommandBuffers(device, m_vulkanResources.getCommandPool(),
			static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
		m_commandBuffers.clear();
	}

	const auto& swapchainImages = m_vulkanSwapchain.getImages();
	const auto& framebuffers = m_vulkanSwapchain.getFramebuffers();
	VkExtent2D extent = m_vulkanSwapchain.getExtent();
	VkRenderPass renderPass = m_vulkanSwapchain.getRenderPass();

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_vulkanResources.getCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	m_commandBuffers.resize(swapchainImages.size());

	for (size_t i = 0; i < m_commandBuffers.size(); ++i)
	{
		if (vkAllocateCommandBuffers(device, &allocInfo, &m_commandBuffers[i]) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to allocate command buffer");
			return false;
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffers[i];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = extent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clearValues[1].depthStencil = {1.0f, 0};
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkBuffer bgBuffer = m_vulkanResources.getBackgroundVertexBuffer();
		if (m_background.shouldRender && m_vulkanPipeline.getBackgroundPipeline() != VK_NULL_HANDLE && bgBuffer != VK_NULL_HANDLE)
		{
			VkBuffer bgBuffers[] = {bgBuffer};
			VkDeviceSize bgOffsets[] = {0};
			vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipeline.getBackgroundPipeline());
			vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, bgBuffers, bgOffsets);
			vkCmdDraw(m_commandBuffers[i], 4, 1, 0, 0);
		}

		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipeline.getGraphicsPipeline());

		if (m_vertexBuffer != VK_NULL_HANDLE && m_vertexCount > 0)
		{
			VkBuffer vertexBuffers[] = {m_vertexBuffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);

			if (m_descriptorSet != VK_NULL_HANDLE)
			{
				vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_vulkanPipeline.getPipelineLayout(), 0, 1, &m_descriptorSet, 0, nullptr);
			}

			struct PushConstants
			{
				int useTexture;
				int enableAlpha;
				float alphaOverride;
			} pc{};
			pc.useTexture = 1;
			pc.enableAlpha = m_icon && m_icon->hasAlpha() ? 1 : 0;
			pc.alphaOverride = 1.0f;

			vkCmdPushConstants(m_commandBuffers[i], m_vulkanPipeline.getPipelineLayout(),
				VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);

			vkCmdDraw(m_commandBuffers[i], m_vertexCount, 1, 0, 0);
		}

		vkCmdEndRenderPass(m_commandBuffers[i]);

		if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
		{
			Logger::error("VK: Failed to record command buffer");
			return false;
		}
	}

	Logger::info("VK: Command buffers created successfully");
	return true;
}

void VulkanRenderer::submitFrame()
{
	try
	{
		VkDevice device = m_vulkanDevice.getDevice();
		VkQueue queue = m_vulkanDevice.getGraphicsQueue();
		VkSwapchainKHR swapchain = m_vulkanSwapchain.getSwapchain();

		if (!m_initialized || swapchain == VK_NULL_HANDLE)
		{
			Logger::error("VK: Not initialized or no swapchain");
			return;
		}

		VkFence fence = m_vulkanResources.getInFlightFence();
		VkSemaphore imageAvailable = m_vulkanResources.getImageAvailableSemaphore();
		VkSemaphore renderFinished = m_vulkanResources.getRenderFinishedSemaphore();

		vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &fence);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			Logger::warn("VK: Swapchain out of date");
			return;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			Logger::error("VK: Failed to acquire next image: {}", static_cast<int>(result));
			return;
		}

		Logger::debug("VK: Acquired image {}, submitting command buffer", imageIndex);

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
			Logger::debug("VK: Swapchain suboptimal or out of date");
			return;
		}

		if (result != VK_SUCCESS)
		{
			Logger::error("VK: Failed to present image: {}", static_cast<int>(result));
		}

		Logger::debug("VK: Frame presented successfully");
	}
	catch (const std::exception& e)
	{
		Logger::error("VK: Exception: {}", e.what());
	}
}