// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

class VulkanDevice;
class VulkanSwapchain;

// Vertex structures for pipelines
struct VulkanVertex
{
	float pos[3];
	float normal[3];
	float texCoord[2];
	uint8_t color[4];
};

struct VulkanBGVertex
{
	float pos[2];
	uint8_t color[4];
};

class VulkanPipeline
{
public:
	VulkanPipeline();
	~VulkanPipeline();

	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline& operator=(const VulkanPipeline&) = delete;

	bool createMainPipeline(VkDevice device, VkRenderPass renderPass, VkExtent2D extent);
	bool createBackgroundPipeline(VkDevice device, VkRenderPass renderPass, VkExtent2D extent);
	void destroy(VkDevice device);

	VkPipeline getGraphicsPipeline() const { return m_graphicsPipeline; }
	VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }
	VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }
	VkPipeline getBackgroundPipeline() const { return m_bgPipeline; }
	VkPipelineLayout getBackgroundPipelineLayout() const { return m_bgPipelineLayout; }

private:
	bool createShaderModules(VkDevice device);
	VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);

	VkShaderModule m_vertexShader = VK_NULL_HANDLE;
	VkShaderModule m_fragmentShader = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

	VkPipelineLayout m_bgPipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_bgPipeline = VK_NULL_HANDLE;
};
