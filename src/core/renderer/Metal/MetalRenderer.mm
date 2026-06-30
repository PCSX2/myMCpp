// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MetalRenderer.h"
#include "MetalDevice.h"
#include "MetalPipeline.h"
#include "MetalResources.h"
#include "Logger.h"
#include "CocoaTools.h"
#include "../../../common/Config.h"
#include "ps2icon.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

struct PushCB
{
	int useTexture;      // 1 = use texture
	int enableAlpha;     // 1 = override alpha
	float alphaOverride; // override value
	float _pad;          // 16-byte alignment
};

struct MetalRendererImpl
{
	MetalDevice device;
	MetalPipeline pipeline;
	MetalResources resources;
	CAMetalLayer* metalLayer = nil;

	void updateBackgroundData(const MetalResources::FrameResources& frameRes, const BackgroundState& background);
	void updateVertexData(const MetalResources::FrameResources& frameRes, PS2Icon::Icon* icon,
		bool animationEnabled, std::chrono::steady_clock::time_point animStart, uint32_t& vertexCount);
	void updateUniformData(const MetalResources::FrameResources& frameRes, const CameraState& camera,
		const LightingState& lighting, uint32_t width, uint32_t height,
		bool animationEnabled, std::chrono::steady_clock::time_point animStart);
};

void MetalRendererImpl::updateBackgroundData(const MetalResources::FrameResources& frameRes, const BackgroundState& background)
{
	struct MetalBGVertex
	{
		float x, y;
		uint8_t r, g, b, a;
	};

	MetalBGVertex bgVerts[4];

	auto setVert = [&](int i, float x, float y, const glm::vec4& c) {
		bgVerts[i].x = x;
		bgVerts[i].y = y;
		bgVerts[i].r = static_cast<uint8_t>(glm::clamp(c.r, 0.0f, 1.0f) * 255.0f);
		bgVerts[i].g = static_cast<uint8_t>(glm::clamp(c.g, 0.0f, 1.0f) * 255.0f);
		bgVerts[i].b = static_cast<uint8_t>(glm::clamp(c.b, 0.0f, 1.0f) * 255.0f);
		bgVerts[i].a = static_cast<uint8_t>(glm::clamp(c.a, 0.0f, 1.0f) * 255.0f);
	};

	setVert(0, -1.0f, 1.0f, background.colors[0]);  // TL
	setVert(1, -1.0f, -1.0f, background.colors[2]); // BL
	setVert(2, 1.0f, 1.0f, background.colors[1]);   // TR
	setVert(3, 1.0f, -1.0f, background.colors[3]);  // BR

	memcpy(frameRes.backgroundBuffer.contents, bgVerts, sizeof(bgVerts));
}

void MetalRendererImpl::updateVertexData(const MetalResources::FrameResources& frameRes, PS2Icon::Icon* icon,
	bool animationEnabled, std::chrono::steady_clock::time_point animStart, uint32_t& vertexCount)
{
	float duration = icon ? static_cast<float>(icon->getFrameLength()) : 1.0f;
	float time = AnimationUtils::computeAnimationTime(
		std::chrono::duration<double>(std::chrono::steady_clock::now() - animStart).count(),
		duration,
		icon ? icon->getAnimSpeed() : 1.0f);

	std::unordered_map<uint32_t, float> weights;
	if (icon && animationEnabled && icon->getAnimationShapes() > 1 && icon->getFrameCount() > 0)
	{
		weights = AnimationUtils::computeShapeWeights(icon, time, duration);
	}
	else
	{
		weights[0] = 1.0f;
	}

	std::vector<AnimationUtils::BlendedVertex> vertices;
	if (icon)
	{
		vertices = AnimationUtils::blendVertices(icon, weights, icon->getVertexCount());
	}
	vertexCount = static_cast<uint32_t>(vertices.size());

	if (vertexCount > 0)
	{
		size_t vertSize = vertices.size() * sizeof(AnimationUtils::BlendedVertex);
		if (vertSize <= frameRes.vertexBuffer.length)
		{
			memcpy(frameRes.vertexBuffer.contents, vertices.data(), vertSize);
		}
	}
}

void MetalRendererImpl::updateUniformData(const MetalResources::FrameResources& frameRes, const CameraState& camera,
	const LightingState& lighting, uint32_t width, uint32_t height,
	bool animationEnabled, std::chrono::steady_clock::time_point animStart)
{
	float autoRotateY = 0.0f;
	if (animationEnabled)
	{
		double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - animStart).count();
		autoRotateY = static_cast<float>(elapsed * -0.5);
	}
	CameraUtils::MatrixSet matrices = CameraUtils::computeMatrices(camera, width, height, autoRotateY);
	UniformBufferUtils::UniformBufferData ubo = UniformBufferUtils::buildUniformBufferData(matrices, lighting, false);

	memcpy(frameRes.uniformBuffer.contents, &ubo, sizeof(ubo));
}

MetalRenderer::MetalRenderer(const WindowInfo& windowInfo, Config* config)
	: m_initialized(false), m_windowInfo(windowInfo), m_vertexCount(0), m_frameCount(0), m_frameIndex(0), m_icon(nullptr), m_animationEnabled(true), m_iconChanged(false), m_animStart(std::chrono::steady_clock::now()), m_config(config), m_impl(std::make_unique<MetalRendererImpl>())
{
	m_camera.applyMode();
	m_lighting.applyMode();
	RendererFactory::registerRenderer(this);
}

MetalRenderer::~MetalRenderer()
{
	RendererFactory::unregisterRenderer(this);
	shutdown();
}

bool MetalRenderer::initialize()
{
	if (m_initialized)
		return true;

	if (!m_impl)
	{
		Logger::error("MTL: Internal renderer state was not created");
		return false;
	}

	if (!m_impl->device.initialize())
	{
		Logger::error("MTL: Failed to initialize device");
		return false;
	}

	if (!CocoaTools::CreateMetalLayer(&m_windowInfo))
	{
		Logger::error("MTL: Failed to create Metal layer");
		return false;
	}

	m_impl->metalLayer = (__bridge CAMetalLayer*)m_windowInfo.surface_handle;
	m_impl->metalLayer.device = m_impl->device.getDevice();
	m_impl->metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
	m_impl->metalLayer.framebufferOnly = YES;

	if (!m_impl->pipeline.initialize(m_impl->device.getDevice()))
	{
		Logger::error("MTL: Failed to initialize pipeline");
		return false;
	}

	if (!m_impl->pipeline.createBackgroundPipeline(m_impl->device.getDevice()))
	{
		Logger::error("MTL: Failed to create background pipeline");
		return false;
	}

	if (!m_impl->resources.initialize(m_impl->device.getDevice()))
	{
		Logger::error("MTL: Failed to initialize resources");
		return false;
	}

	resize(m_windowInfo.surface_width, m_windowInfo.surface_height);

	if (m_config)
	{
		setVSync(m_config->getVSync());
	}

	m_initialized = true;
	return true;
}

void MetalRenderer::shutdown()
{
	if (m_initialized && m_impl)
	{
		CocoaTools::DestroyMetalLayer(&m_windowInfo);
		m_impl->resources.shutdown();
		m_impl->pipeline.shutdown();
		m_impl->device.shutdown();
		m_impl->metalLayer = nil;
		m_initialized = false;
	}
}

void MetalRenderer::setIcon(std::shared_ptr<PS2Icon::Icon> icon)
{
	m_icon = icon;
	if (m_icon)
	{
		m_iconChanged = true;
		m_animStart = std::chrono::steady_clock::now();
	}
}

bool MetalRenderer::hasValidIcon() const
{
	return m_icon && m_icon->isValid();
}

void MetalRenderer::render()
{
	if (!m_initialized || !m_impl || !m_impl->metalLayer || !hasValidIcon())
		return;

	dispatch_semaphore_wait(m_impl->resources.getSemaphore(m_frameIndex), DISPATCH_TIME_FOREVER);

	id<CAMetalDrawable> drawable = [m_impl->metalLayer nextDrawable];
	if (!drawable)
	{
		dispatch_semaphore_signal(m_impl->resources.getSemaphore(m_frameIndex));
		return;
	}

	if (m_iconChanged && m_icon && m_icon->getTextureData() != nullptr)
	{
		uint32_t width = m_icon->getTextureWidth();
		uint32_t height = m_icon->getTextureHeight();
		auto rgbaData = TextureUtils::convertPS2TextureToRGBA(m_icon->getTextureData(), width, height);
		if (!rgbaData.empty())
		{
			m_impl->resources.uploadTexture(m_impl->device.getDevice(), rgbaData.data(), width, height);
		}
		else
		{
			Logger::warn("MTL: Texture conversion returned empty data");
		}
		m_iconChanged = false;
	}

	const MetalResources::FrameResources& frameRes = m_impl->resources.getFrameResources(m_frameIndex);
	m_impl->updateBackgroundData(frameRes, m_background);
	m_impl->updateVertexData(frameRes, m_icon.get(), m_animationEnabled, m_animStart, m_vertexCount);
	m_impl->updateUniformData(frameRes, m_camera, m_lighting, m_windowInfo.surface_width, m_windowInfo.surface_height, m_animationEnabled, m_animStart);

	MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
	passDesc.colorAttachments[0].texture = drawable.texture;
	passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
	passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
	passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;

	if (m_impl->resources.getDepthTexture())
	{
		passDesc.depthAttachment.texture = m_impl->resources.getDepthTexture();
		passDesc.depthAttachment.loadAction = MTLLoadActionClear;
		passDesc.depthAttachment.clearDepth = 1.0;
		passDesc.depthAttachment.storeAction = MTLStoreActionDontCare;
	}

	id<MTLCommandBuffer> commandBuffer = [m_impl->device.getCommandQueue() commandBuffer];
	if (!commandBuffer)
	{
		dispatch_semaphore_signal(m_impl->resources.getSemaphore(m_frameIndex));
		Logger::error("MTL: Failed to acquire command buffer");
		return;
	}

	id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
	if (!encoder)
	{
		dispatch_semaphore_signal(m_impl->resources.getSemaphore(m_frameIndex));
		Logger::error("MTL: Failed to create render command encoder");
		return;
	}

	if (m_background.shouldRender && m_impl->pipeline.getBackgroundPipelineState())
	{
		[encoder setRenderPipelineState:m_impl->pipeline.getBackgroundPipelineState()];
		[encoder setDepthStencilState:m_impl->pipeline.getBackgroundDepthStencilState()];

		[encoder setVertexBuffer:frameRes.backgroundBuffer offset:0 atIndex:0];
		[encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
	}

	if (m_vertexCount > 0)
	{
		[encoder setRenderPipelineState:m_impl->pipeline.getPipelineState()];
		[encoder setDepthStencilState:m_impl->pipeline.getDepthStencilState()];
		[encoder setFrontFacingWinding:MTLWindingCounterClockwise];
		[encoder setCullMode:MTLCullModeBack];

		[encoder setVertexBuffer:frameRes.uniformBuffer offset:0 atIndex:0];
		[encoder setVertexBuffer:frameRes.vertexBuffer offset:0 atIndex:2];

		PushCB pushCB;
		pushCB.useTexture = (m_impl->resources.getTexture() != nil) ? 1 : 0;
		pushCB.enableAlpha = (m_icon && m_icon->hasAlpha()) ? 1 : 0;
		pushCB.alphaOverride = 1.0f;
		pushCB._pad = 0.0f;

		[encoder setFragmentBytes:&pushCB length:sizeof(PushCB) atIndex:1];
		[encoder setFragmentBuffer:frameRes.uniformBuffer offset:0 atIndex:0];

		if (m_impl->resources.getTexture())
		{
			[encoder setFragmentTexture:m_impl->resources.getTexture() atIndex:0];
			[encoder setFragmentSamplerState:m_impl->resources.getSamplerState() atIndex:0];
		}

		[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:m_vertexCount];
	}

	[encoder endEncoding];
	[commandBuffer presentDrawable:drawable];

	dispatch_semaphore_t semaphore = m_impl->resources.getSemaphore(m_frameIndex);
	[commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> cmdBuf) {
	  (void)cmdBuf;
	  dispatch_semaphore_signal(semaphore);
	}];

	[commandBuffer commit];

	m_frameCount++;
	m_frameIndex = (m_frameIndex + 1) % kMaxFramesInFlight;
}

void MetalRenderer::resize(uint32_t width, uint32_t height)
{
	m_windowInfo.surface_width = width;
	m_windowInfo.surface_height = height;

	if (!m_impl)
		return;

	CocoaTools::SetDrawableSize(&m_windowInfo, width, height);

	if (width == 0 || height == 0)
	{
		return;
	}

	if (!m_impl->resources.createDepthTexture(m_impl->device.getDevice(), width, height))
	{
		Logger::error("MTL: Failed to create depth texture {}x{}", width, height);
	}
}

void MetalRenderer::setRotation(float x, float y, float z) { m_camera.setRotation(x, y, z); }
void MetalRenderer::setZoom(float zoom) { m_camera.setZoom(zoom); }
void MetalRenderer::setCameraOffset(float x, float y, float z) { m_camera.setOffset(x, y, z); }
void MetalRenderer::resetCamera() { m_camera.reset(); }
void MetalRenderer::setCameraMode(CameraMode mode)
{
	m_camera.mode = mode;
	m_camera.applyMode();
}
void MetalRenderer::setLightingFromIconSys(PS2IconSys* iconSys) { m_lighting.loadFromIconSys(iconSys); }
void MetalRenderer::setLightingMode(LightingMode mode)
{
	m_lighting.mode = mode;
	m_lighting.applyMode();
}
void MetalRenderer::setBackgroundFromIconSys(PS2IconSys* iconSys) { m_background.loadFromIconSys(iconSys); }
void MetalRenderer::setBackgroundColor(float r, float g, float b, float a) { m_background.setColor(r, g, b, a); }

void MetalRenderer::setVSync(bool enabled)
{
	if (m_impl && m_impl->metalLayer && [m_impl->metalLayer respondsToSelector:@selector(setDisplaySyncEnabled:)])
	{
		[m_impl->metalLayer setDisplaySyncEnabled:(enabled ? YES : NO)];
		Logger::info("MTL: VSync set to {}", enabled ? "enabled" : "disabled");

		if (m_config)
		{
			m_config->setVSync(enabled);
		}
	}
	else
	{
		Logger::warn("MTL: Cannot set VSync displaySyncEnabled not available");
	}
}
