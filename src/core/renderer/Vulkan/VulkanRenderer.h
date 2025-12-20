// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "../Renderer.h"
#include "WindowInfo.h"
#include "../Common/RendererCommon.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanResources.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <cstdint>
#include <chrono>

namespace PS2Icon
{
	class Icon;
	struct VertexCoord;
	struct NormalUV;
	struct VertexColor;
} // namespace PS2Icon

class VulkanRenderer : public Renderer
{
public:
	explicit VulkanRenderer(const WindowInfo& windowInfo);
	virtual ~VulkanRenderer();

	bool initialize() override;
	void shutdown() override;
	bool isInitialized() const override { return m_initialized; }

	void setIcon(std::shared_ptr<PS2Icon::Icon> icon) override;
	bool hasValidIcon() const override;

	void setAnimationEnabled(bool enabled) override;
	bool isAnimationEnabled() const override { return m_animationEnabled; }

	void setRotation(float x, float y, float z) override;
	void setZoom(float zoom) override;
	void setCameraOffset(float x, float y, float z) override;
	void resetCamera() override;

	void setCameraMode(CameraMode mode) override;

	void setLightingFromIconSys(class PS2IconSys* iconSys) override;
	void setLightingMode(LightingMode mode) override;
	void setBackgroundFromIconSys(class PS2IconSys* iconSys) override;
	void setBackgroundColor(float r, float g, float b, float a = 1.0f) override;

	void render() override;
	void resize(uint32_t width, uint32_t height) override;

	uint32_t getVertexCount() const override;
	uint32_t getFrameCount() const override;

private:
	WindowInfo m_windowInfo;
	uint32_t m_width;
	uint32_t m_height;
	bool m_initialized;

	VulkanDevice m_vulkanDevice;
	VulkanSwapchain m_vulkanSwapchain;
	VulkanPipeline m_vulkanPipeline;
	VulkanResources m_vulkanResources;

	std::vector<VkCommandBuffer> m_commandBuffers;

	std::shared_ptr<PS2Icon::Icon> m_icon;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexMemory;
	VkBuffer m_uniformBuffer;
	VkDeviceMemory m_uniformMemory;

	VkImage m_textureImage;
	VkDeviceMemory m_textureMemory;
	VkImageView m_textureView;
	VkSampler m_textureSampler;

	VkDescriptorPool m_descriptorPool;
	VkDescriptorSet m_descriptorSet;

	bool m_iconChanged;
	bool m_animationEnabled;
	std::chrono::steady_clock::time_point m_animStart;

	uint32_t m_vertexCount;

	LightingState m_lighting;
	CameraState m_camera;
	BackgroundState m_background;

	bool createCommandBuffers();
	bool uploadTexture();
	void updateBackgroundVertexData();

	void prepareVertexData();
	void updateUniformBuffer();
	void submitFrame();
};
