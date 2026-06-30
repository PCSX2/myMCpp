// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MetalResources.h"
#include "../Common/RendererCommon.h"

namespace
{
	constexpr size_t kMaxVertexBufferBytes = 4 * 1024 * 1024;
	constexpr size_t kUniformBufferBytes = sizeof(UniformBufferUtils::UniformBufferData);
	constexpr size_t kBackgroundBufferBytes = 4 * (2 * sizeof(float) + 4 * sizeof(uint8_t));
} // namespace

MetalResources::MetalResources()
{
}

MetalResources::~MetalResources()
{
	shutdown();
}

bool MetalResources::initialize(id<MTLDevice> device)
{
	m_error.Clear();

	for (int i = 0; i < kMaxFramesInFlight; i++)
	{
		m_renderSemaphores[i] = dispatch_semaphore_create(1);
	}

	for (int i = 0; i < kMaxFramesInFlight; i++)
	{
		m_frames[i].vertexBuffer = [device newBufferWithLength:kMaxVertexBufferBytes options:MTLResourceStorageModeShared];
		m_frames[i].uniformBuffer = [device newBufferWithLength:kUniformBufferBytes options:MTLResourceStorageModeShared];
		m_frames[i].backgroundBuffer = [device newBufferWithLength:kBackgroundBufferBytes options:MTLResourceStorageModeShared];

		if (!m_frames[i].vertexBuffer || !m_frames[i].uniformBuffer || !m_frames[i].backgroundBuffer)
			return m_error.Fail("MTL: Failed to create frame buffers");
	}

	MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
	samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
	samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
	samplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
	samplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
	m_samplerState = [device newSamplerStateWithDescriptor:samplerDesc];

	if (!m_samplerState)
		return m_error.Fail("MTL: Failed to create sampler state");

	return true;
}

void MetalResources::shutdown()
{
	m_texture = nil;
	m_samplerState = nil;
	m_depthTexture = nil;
	for (int i = 0; i < kMaxFramesInFlight; i++)
	{
		m_renderSemaphores[i] = nil;
		m_frames[i].vertexBuffer = nil;
		m_frames[i].uniformBuffer = nil;
		m_frames[i].backgroundBuffer = nil;
	}
}

bool MetalResources::createDepthTexture(id<MTLDevice> device, uint32_t width, uint32_t height)
{
	if (!device || width == 0 || height == 0)
	{
		m_depthTexture = nil;
		return false;
	}

	MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
																						 width:width
																						height:height
																					 mipmapped:NO];
	depthDesc.usage = MTLTextureUsageRenderTarget;
	depthDesc.storageMode = MTLStorageModePrivate;

	m_depthTexture = [device newTextureWithDescriptor:depthDesc];
	return m_depthTexture != nil;
}

void MetalResources::uploadTexture(id<MTLDevice> device, const uint8_t* rgbaData, uint32_t width, uint32_t height)
{
	if (!device || !rgbaData || width == 0 || height == 0)
	{
		return;
	}

	MTLTextureDescriptor* texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB
																					   width:width
																					  height:height
																				   mipmapped:NO];
	texDesc.usage = MTLTextureUsageShaderRead;
	m_texture = [device newTextureWithDescriptor:texDesc];

	if (!m_texture)
	{
		return;
	}

	MTLRegion region = MTLRegionMake2D(0, 0, width, height);
	[m_texture replaceRegion:region mipmapLevel:0 withBytes:rgbaData bytesPerRow:width * 4];
}
