// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "VulkanPipeline.h"

#include <array>
#include <fstream>
#include <sstream>
#include "Logger.h"
#include "ResourcePath.h"
#include <vector>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

namespace
{
	bool InitializeGlslang()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = glslang::InitializeProcess();
			if (!initialized)
				Logger::error("VK: Failed to initialize glslang");
		}
		return initialized;
	}

	EShLanguage GetShaderStage(const char* filename)
	{
		if (filename == nullptr)
			return EShLangVertex;

		std::string name(filename);
		if (name.ends_with(".vert.glsl"))
			return EShLangVertex;
		if (name.ends_with(".frag.glsl"))
			return EShLangFragment;
		return EShLangVertex;
	}

	bool CompileGlslToSpirv(const fs::path& path, std::vector<uint32_t>& spirv)
	{
		if (!InitializeGlslang())
			return false;

		std::ifstream file(path);
		if (!file.is_open())
		{
			Logger::error("VK: Failed to open GLSL shader file: {}", path.string());
			return false;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		const std::string source = buffer.str();

		const std::string pathStr = path.string();
		const char* cstr = source.c_str();

		EShLanguage stage = GetShaderStage(pathStr.c_str());
		glslang::TShader shader(stage);
		shader.setStrings(&cstr, 1);
		shader.setEntryPoint("main");
		shader.setSourceEntryPoint("main");
		shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 1);
		shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
		shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

		EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

		const TBuiltInResource* resources = GetDefaultResources();
		if (!resources)
		{
			Logger::error("VK: glslang::GetDefaultResources() returned null for {}", pathStr);
			return false;
		}

		if (!shader.parse(resources, 100, false, messages))
		{
			Logger::error("VK: GLSL shader parse failed for {}:\n{}", pathStr, shader.getInfoLog());
			return false;
		}

		glslang::TProgram program;
		program.addShader(&shader);

		if (!program.link(messages))
		{
			Logger::error("VK: GLSL program link failed for {}:\n{}", pathStr, program.getInfoLog());
			return false;
		}

		glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
		return true;
	}
} // namespace

VulkanPipeline::VulkanPipeline() = default;

VulkanPipeline::~VulkanPipeline() = default;

void VulkanPipeline::destroy(VkDevice device)
{
	if (m_bgPipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device, m_bgPipeline, nullptr);
		m_bgPipeline = VK_NULL_HANDLE;
	}
	if (m_bgPipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(device, m_bgPipelineLayout, nullptr);
		m_bgPipelineLayout = VK_NULL_HANDLE;
	}
	if (m_graphicsPipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device, m_graphicsPipeline, nullptr);
		m_graphicsPipeline = VK_NULL_HANDLE;
	}
	if (m_pipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
		m_pipelineLayout = VK_NULL_HANDLE;
	}
	if (m_descriptorSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
		m_descriptorSetLayout = VK_NULL_HANDLE;
	}
	if (m_fragmentShader != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(device, m_fragmentShader, nullptr);
		m_fragmentShader = VK_NULL_HANDLE;
	}
	if (m_vertexShader != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(device, m_vertexShader, nullptr);
		m_vertexShader = VK_NULL_HANDLE;
	}
	if (m_pipelineCache != VK_NULL_HANDLE)
	{
		vkDestroyPipelineCache(device, m_pipelineCache, nullptr);
		m_pipelineCache = VK_NULL_HANDLE;
	}
}

bool VulkanPipeline::createPipelineCache(VkDevice device)
{
	VkPipelineCacheCreateInfo cacheInfo{};
	cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheInfo.initialDataSize = 0;
	cacheInfo.pInitialData = nullptr;

	if (vkCreatePipelineCache(device, &cacheInfo, nullptr, &m_pipelineCache) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create pipeline cache");
		return false;
	}

	return true;
}

VkShaderModule VulkanPipeline::createShaderModule(VkDevice device, const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create shader module");
		return VK_NULL_HANDLE;
	}

	return shaderModule;
}

bool VulkanPipeline::createShaderModules(VkDevice device)
{
	fs::path vertPath = ResourcePath::shaders() / "Vulkan" / "icon.vert.glsl";
	fs::path fragPath = ResourcePath::shaders() / "Vulkan" / "icon.frag.glsl";

	std::vector<uint32_t> vertSpirv;
	std::vector<uint32_t> fragSpirv;

	if (!CompileGlslToSpirv(vertPath, vertSpirv))
		return false;
	if (!CompileGlslToSpirv(fragPath, fragSpirv))
		return false;

	std::vector<char> vertCode(reinterpret_cast<char*>(vertSpirv.data()),
		reinterpret_cast<char*>(vertSpirv.data()) + vertSpirv.size() * sizeof(uint32_t));
	std::vector<char> fragCode(reinterpret_cast<char*>(fragSpirv.data()),
		reinterpret_cast<char*>(fragSpirv.data()) + fragSpirv.size() * sizeof(uint32_t));

	m_vertexShader = createShaderModule(device, vertCode);
	m_fragmentShader = createShaderModule(device, fragCode);

	if (m_vertexShader == VK_NULL_HANDLE || m_fragmentShader == VK_NULL_HANDLE)
	{
		Logger::error("VK: Failed to create shader modules");
		return false;
	}

	Logger::info("VK: Shader modules created successfully");
	return true;
}

bool VulkanPipeline::createMainPipeline(VkDevice device, VkRenderPass renderPass)
{
	if (!createShaderModules(device))
		return false;

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = m_vertexShader;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = m_fragmentShader;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(VulkanVertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attributeDescriptions[4]{};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(VulkanVertex, pos);
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(VulkanVertex, normal);
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(VulkanVertex, texCoord);
	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R8G8B8A8_UNORM;
	attributeDescriptions[3].offset = offsetof(VulkanVertex, color);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = 4;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = nullptr;
	viewportState.scissorCount = 1;
	viewportState.pScissors = nullptr;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding bindings[] = {uboLayoutBinding, samplerLayoutBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create descriptor set layout");
		return false;
	}

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(uint32_t) * 3;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create pipeline layout");
		return false;
	}

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	std::array<VkDynamicState, 2> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create graphics pipeline");
		return false;
	}

	return true;
}

bool VulkanPipeline::createBackgroundPipeline(VkDevice device, VkRenderPass renderPass)
{
	fs::path vertPath = ResourcePath::shaders() / "Vulkan" / "background.vert.glsl";
	fs::path fragPath = ResourcePath::shaders() / "Vulkan" / "background.frag.glsl";

	std::vector<uint32_t> vertSpirv;
	std::vector<uint32_t> fragSpirv;

	if (!CompileGlslToSpirv(vertPath, vertSpirv))
		return false;
	if (!CompileGlslToSpirv(fragPath, fragSpirv))
		return false;

	std::vector<char> vertCode(reinterpret_cast<char*>(vertSpirv.data()),
		reinterpret_cast<char*>(vertSpirv.data()) + vertSpirv.size() * sizeof(uint32_t));
	std::vector<char> fragCode(reinterpret_cast<char*>(fragSpirv.data()),
		reinterpret_cast<char*>(fragSpirv.data()) + fragSpirv.size() * sizeof(uint32_t));

	VkShaderModule bgVert = createShaderModule(device, vertCode);
	VkShaderModule bgFrag = createShaderModule(device, fragCode);
	if (bgVert == VK_NULL_HANDLE || bgFrag == VK_NULL_HANDLE)
	{
		Logger::error("VK: Failed to create background shader modules");
		return false;
	}

	VkPipelineShaderStageCreateInfo stages[2]{};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = bgVert;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = bgFrag;
	stages[1].pName = "main";

	VkVertexInputBindingDescription binding{};
	binding.binding = 0;
	binding.stride = sizeof(VulkanBGVertex);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attrs[2]{};
	attrs[0].binding = 0;
	attrs[0].location = 0;
	attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
	attrs[0].offset = offsetof(VulkanBGVertex, pos);
	attrs[1].binding = 0;
	attrs[1].location = 1;
	attrs[1].format = VK_FORMAT_R8G8B8A8_UNORM;
	attrs[1].offset = offsetof(VulkanBGVertex, color);

	VkPipelineVertexInputStateCreateInfo vi{};
	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vi.vertexBindingDescriptionCount = 1;
	vi.pVertexBindingDescriptions = &binding;
	vi.vertexAttributeDescriptionCount = 2;
	vi.pVertexAttributeDescriptions = attrs;

	VkPipelineInputAssemblyStateCreateInfo ia{};
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	ia.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo vp{};
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.viewportCount = 1;
	vp.pViewports = nullptr;
	vp.scissorCount = 1;
	vp.pScissors = nullptr;

	VkPipelineRasterizationStateCreateInfo rs{};
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.depthClampEnable = VK_FALSE;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.polygonMode = VK_POLYGON_MODE_FILL;
	rs.lineWidth = 1.0f;
	rs.cullMode = VK_CULL_MODE_NONE;
	rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rs.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo ms{};
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.sampleShadingEnable = VK_FALSE;
	ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState blend{};
	blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blend.blendEnable = VK_TRUE;
	blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend.colorBlendOp = VK_BLEND_OP_ADD;
	blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blend.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo cb{};
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cb.logicOpEnable = VK_FALSE;
	cb.attachmentCount = 1;
	cb.pAttachments = &blend;

	VkPipelineLayoutCreateInfo pl{};
	pl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pl.setLayoutCount = 0;
	pl.pSetLayouts = nullptr;
	pl.pushConstantRangeCount = 0;
	pl.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device, &pl, nullptr, &m_bgPipelineLayout) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create background pipeline layout");
		vkDestroyShaderModule(device, bgVert, nullptr);
		vkDestroyShaderModule(device, bgFrag, nullptr);
		return false;
	}

	VkPipelineDepthStencilStateCreateInfo bgDepth{};
	bgDepth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	bgDepth.depthTestEnable = VK_FALSE;
	bgDepth.depthWriteEnable = VK_FALSE;
	bgDepth.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	bgDepth.depthBoundsTestEnable = VK_FALSE;
	bgDepth.stencilTestEnable = VK_FALSE;

	std::array<VkDynamicState, 2> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pi{};
	pi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pi.stageCount = 2;
	pi.pStages = stages;
	pi.pVertexInputState = &vi;
	pi.pInputAssemblyState = &ia;
	pi.pViewportState = &vp;
	pi.pRasterizationState = &rs;
	pi.pMultisampleState = &ms;
	pi.pDepthStencilState = &bgDepth;
	pi.pColorBlendState = &cb;
	pi.pDynamicState = &dynamicState;
	pi.layout = m_bgPipelineLayout;
	pi.renderPass = renderPass;
	pi.subpass = 0;

	if (vkCreateGraphicsPipelines(device, m_pipelineCache, 1, &pi, nullptr, &m_bgPipeline) != VK_SUCCESS)
	{
		Logger::error("VK: Failed to create background pipeline");
		vkDestroyPipelineLayout(device, m_bgPipelineLayout, nullptr);
		m_bgPipelineLayout = VK_NULL_HANDLE;
		vkDestroyShaderModule(device, bgVert, nullptr);
		vkDestroyShaderModule(device, bgFrag, nullptr);
		return false;
	}

	vkDestroyShaderModule(device, bgVert, nullptr);
	vkDestroyShaderModule(device, bgFrag, nullptr);
	return true;
}
