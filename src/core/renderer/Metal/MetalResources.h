// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <Metal/Metal.h>
#include <dispatch/dispatch.h>
#include <vector>
#include <cstdint>
#include <array>

static const int kMaxFramesInFlight = 3;

class MetalResources
{
public:
    MetalResources();
    ~MetalResources();
    
    bool initialize(id<MTLDevice> device);
    void shutdown();
    
    void uploadTexture(id<MTLDevice> device, const uint8_t* rgbaData, uint32_t width, uint32_t height);
    id<MTLTexture> getTexture() const { return m_texture; }
    
    bool createDepthTexture(id<MTLDevice> device, uint32_t width, uint32_t height);
    id<MTLTexture> getDepthTexture() const { return m_depthTexture; }
    
    dispatch_semaphore_t getSemaphore(uint32_t index) const { return m_renderSemaphores[index]; }
    
    struct FrameResources {
        id<MTLBuffer> vertexBuffer;
        id<MTLBuffer> uniformBuffer;
        id<MTLBuffer> backgroundBuffer;
    };
    
    const FrameResources& getFrameResources(uint32_t index) const { return m_frames[index]; }
    
private:
    id<MTLTexture> m_texture = nil;
    id<MTLTexture> m_depthTexture = nil;
    
    std::array<dispatch_semaphore_t, kMaxFramesInFlight> m_renderSemaphores;
    std::array<FrameResources, kMaxFramesInFlight> m_frames;
};
