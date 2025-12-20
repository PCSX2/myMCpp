// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once


#include <Metal/Metal.h>

class MetalPipeline
{
public:
    MetalPipeline();
    ~MetalPipeline();
    
    bool initialize(id<MTLDevice> device);
    bool createBackgroundPipeline(id<MTLDevice> device);
    void shutdown();
    
    id<MTLRenderPipelineState> getPipelineState() const { return m_pipelineState; }
    id<MTLRenderPipelineState> getBackgroundPipelineState() const { return m_bgPipelineState; }
    id<MTLDepthStencilState> getDepthStencilState() const { return m_depthStencilState; }
    id<MTLDepthStencilState> getBackgroundDepthStencilState() const { return m_bgDepthStencilState; }
    
private:
    id<MTLLibrary> m_library = nil;
    id<MTLRenderPipelineState> m_pipelineState = nil;
    id<MTLRenderPipelineState> m_bgPipelineState = nil;
    id<MTLDepthStencilState> m_depthStencilState = nil;
    id<MTLDepthStencilState> m_bgDepthStencilState = nil;
};
