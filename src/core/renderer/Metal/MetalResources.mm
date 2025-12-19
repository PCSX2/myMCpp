// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MetalResources.h"

MetalResources::MetalResources()
{
}

MetalResources::~MetalResources()
{
    shutdown();
}

bool MetalResources::initialize(id<MTLDevice> device)
{
    for (int i = 0; i < kMaxFramesInFlight; i++) {
        m_renderSemaphores[i] = dispatch_semaphore_create(kMaxFramesInFlight);
    }

    const size_t MAX_VERTICES_SIZE = 65536;
    const size_t UBO_SIZE = 1024; // Matrices + Lighting
    const size_t BG_SIZE = 4 * 32; // 4 verts * stride (approx)

    for (int i = 0; i < kMaxFramesInFlight; i++) {
        m_frames[i].vertexBuffer = [device newBufferWithLength:MAX_VERTICES_SIZE options:MTLResourceStorageModeShared];
        m_frames[i].uniformBuffer = [device newBufferWithLength:UBO_SIZE options:MTLResourceStorageModeShared];
        m_frames[i].backgroundBuffer = [device newBufferWithLength:BG_SIZE options:MTLResourceStorageModeShared];

        if (!m_frames[i].vertexBuffer || !m_frames[i].uniformBuffer || !m_frames[i].backgroundBuffer) {
            return false;
        }
    }

    return true;
}

void MetalResources::shutdown()
{
    m_texture = nil;
    m_depthTexture = nil;
    for (int i = 0; i < kMaxFramesInFlight; i++) {
        m_renderSemaphores[i] = nil;
        m_frames[i].vertexBuffer = nil;
        m_frames[i].uniformBuffer = nil;
        m_frames[i].backgroundBuffer = nil;
    }
}

bool MetalResources::createDepthTexture(id<MTLDevice> device, uint32_t width, uint32_t height)
{
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
    MTLTextureDescriptor* texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                       width:width
                                                                                      height:height
                                                                                   mipmapped:NO];
    m_texture = [device newTextureWithDescriptor:texDesc];

    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [m_texture replaceRegion:region mipmapLevel:0 withBytes:rgbaData bytesPerRow:width * 4];
}
