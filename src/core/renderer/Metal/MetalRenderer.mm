// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MetalRenderer.h"
#include "../../common/Logger.h"
#include "../../common/CocoaTools.h"
#include "../../core/formats/ps2icon.h"

struct PushCB
{
    int   useTexture;      // 1 = use texture
    int   enableAlpha;     // 1 = override alpha
    float alphaOverride;  // override value
    float _pad;            // 16-byte alignment
};

MetalRenderer::MetalRenderer(const WindowInfo& windowInfo)
	: m_windowInfo(windowInfo)
    , m_animStart(std::chrono::steady_clock::now())
{
}

MetalRenderer::~MetalRenderer()
{
	shutdown();
}

bool MetalRenderer::initialize()
{
    if (m_initialized) return true;
    
    if (!m_device.initialize()) {
        Logger::error("MTL: Failed to initialize device");
        return false;
    }
    
    if (!CocoaTools::CreateMetalLayer(&m_windowInfo)) {
        Logger::error("MTL: Failed to create Metal layer");
        return false;
    }
    
    m_metalLayer = (__bridge CAMetalLayer*)m_windowInfo.surface_handle;
    m_metalLayer.device = m_device.getDevice();
    m_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    m_metalLayer.framebufferOnly = YES;
    
    if (!m_pipeline.initialize(m_device.getDevice())) {
        Logger::error("MTL: Failed to initialize pipeline");
        return false;
    }
    
    if (!m_pipeline.createBackgroundPipeline(m_device.getDevice())) {
        Logger::error("MTL: Failed to create background pipeline");
        return false;
    }
    
    if (!m_resources.initialize(m_device.getDevice())) {
        Logger::error("MTL: Failed to initialize resources");
        return false;
    }
    
    // Initial size setup
    resize(m_windowInfo.surface_width, m_windowInfo.surface_height);
    
    m_initialized = true;
    return true;
}

void MetalRenderer::shutdown()
{
    if (m_initialized) {
        CocoaTools::DestroyMetalLayer(&m_windowInfo);
        m_resources.shutdown();
        m_pipeline.shutdown();
        m_device.shutdown();
        m_metalLayer = nil;
        m_initialized = false;
    }
}

void MetalRenderer::setIcon(std::shared_ptr<PS2Icon::Icon> icon)
{
    m_icon = icon;
    if (m_icon) {
        m_iconChanged = true;
        // Reset animation time
        m_animStart = std::chrono::steady_clock::now();
    }
}

void MetalRenderer::render()
{
    if (!m_initialized || !m_metalLayer) return;
    
    dispatch_semaphore_wait(m_resources.getSemaphore(m_frameIndex), DISPATCH_TIME_FOREVER);
    
    id<CAMetalDrawable> drawable = [m_metalLayer nextDrawable];
    if (!drawable) {
        dispatch_semaphore_signal(m_resources.getSemaphore(m_frameIndex));
        return;
    }
    
    if (m_iconChanged && m_icon && !m_icon->textureData.empty()) {
         uint32_t width = m_icon->texWidth;
         uint32_t height = m_icon->texHeight;
         auto rgbaData = TextureUtils::convertPS2TextureToRGBA(reinterpret_cast<const uint16_t*>(m_icon->textureData.data()), width, height);
         m_resources.uploadTexture(m_device.getDevice(), rgbaData.data(), width, height);
         m_iconChanged = false;
    }
    
    updateBackgroundData(frameRes);
    updateVertexData(frameRes);
    updateUniformData(frameRes);
        
    MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    passDesc.colorAttachments[0].texture = drawable.texture;
    passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0,0,0,1); // Clear to black, background shader covers it
    passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    
    // Depth Attachment
    if (m_resources.getDepthTexture()) {
        passDesc.depthAttachment.texture = m_resources.getDepthTexture();
        passDesc.depthAttachment.loadAction = MTLLoadActionClear;
        passDesc.depthAttachment.clearDepth = 1.0;
        passDesc.depthAttachment.storeAction = MTLStoreActionDontCare;
    }
    
    id<MTLCommandBuffer> commandBuffer = [m_device.getCommandQueue() commandBuffer];
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
    
    // Draw Background
    if (m_pipeline.getBackgroundPipelineState()) {
        [encoder setRenderPipelineState:m_pipeline.getBackgroundPipelineState()];
        [encoder setDepthStencilState:m_pipeline.getBackgroundDepthStencilState()];
        
        [encoder setVertexBuffer:frameRes.backgroundBuffer offset:0 atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    }
    
    // Draw Icon
    if (m_vertexCount > 0) {
        [encoder setRenderPipelineState:m_pipeline.getPipelineState()];
        [encoder setDepthStencilState:m_pipeline.getDepthStencilState()];
        
        [encoder setVertexBuffer:frameRes.uniformBuffer offset:0 atIndex:0];
        [encoder setVertexBuffer:frameRes.vertexBuffer offset:0 atIndex:2];
        
        PushCB pushCB;
        pushCB.useTexture = (m_resources.getTexture() != nil) ? 1 : 0;
        pushCB.enableAlpha = 0;
        pushCB.alphaOverride = 1.0f;
        pushCB._pad = 0.0f;
        
        [encoder setFragmentBytes:&pushCB length:sizeof(PushCB) atIndex:1];
        [encoder setFragmentBuffer:frameRes.uniformBuffer offset:0 atIndex:0];
        
        if (m_resources.getTexture()) {
            [encoder setFragmentTexture:m_resources.getTexture() atIndex:0];
        }
        
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:m_vertexCount];
    }
    
    [encoder endEncoding];
    [commandBuffer presentDrawable:drawable];
    
    // Add completion handler to signal semaphore
    id<MTLSemaphore> semaphore = m_resources.getSemaphore(m_frameIndex);
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> cmdBuf) {
        dispatch_semaphore_signal(semaphore);
    }];
    
    [commandBuffer commit];
    
    m_frameCount++;
    m_frameIndex = (m_frameIndex + 1) % kMaxFramesInFlight;
}

void MetalRenderer::updateBackgroundData(const MetalResources::FrameResources& frameRes)
{
    struct MetalBGVertex {
        float x, y;
        uint8_t r, g, b, a;
    };
    
    MetalBGVertex bgVerts[4];
    
    auto setVert = [&](int i, float x, float y, const glm::vec4& c) {
        bgVerts[i].x = x; bgVerts[i].y = y;
        bgVerts[i].r = static_cast<uint8_t>(glm::clamp(c.r, 0.0f, 1.0f) * 255.0f);
        bgVerts[i].g = static_cast<uint8_t>(glm::clamp(c.g, 0.0f, 1.0f) * 255.0f);
        bgVerts[i].b = static_cast<uint8_t>(glm::clamp(c.b, 0.0f, 1.0f) * 255.0f);
        bgVerts[i].a = static_cast<uint8_t>(glm::clamp(c.a, 0.0f, 1.0f) * 255.0f);
    };
    
    // Triangle Strip Quad
    setVert(0, -1.0f,  1.0f, m_background.colors[0]); // TL
    setVert(1, -1.0f, -1.0f, m_background.colors[2]); // BL
    setVert(2,  1.0f,  1.0f, m_background.colors[1]); // TR
    setVert(3,  1.0f, -1.0f, m_background.colors[3]); // BR
    
    memcpy(frameRes.backgroundBuffer.contents, bgVerts, sizeof(bgVerts));
    [frameRes.backgroundBuffer didModifyRange:NSMakeRange(0, sizeof(bgVerts))];
}

void MetalRenderer::updateVertexData(const MetalResources::FrameResources& frameRes)
{
    float time = AnimationUtils::computeAnimationTime(
        std::chrono::duration<double>(std::chrono::steady_clock::now() - m_animStart).count(),
        m_icon ? m_icon->frameLength : 1.0f);
    
    std::unordered_map<uint32_t, float> weights;
    if (m_icon && m_animationEnabled) {
        weights = AnimationUtils::computeShapeWeights(m_icon.get(), time, m_icon ? m_icon->length : 1.0f);
    } else {
        weights[0] = 1.0f;
    }
    
    std::vector<AnimationUtils::BlendedVertex> vertices;
    if (m_icon) {
        vertices = AnimationUtils::blendVertices(m_icon.get(), weights, m_icon->vertexCount);
    }
    m_vertexCount = vertices.size();
    
    if (m_vertexCount > 0) {
        size_t vertSize = vertices.size() * sizeof(AnimationUtils::BlendedVertex);
        if (vertSize <= frameRes.vertexBuffer.length) {
            memcpy(frameRes.vertexBuffer.contents, vertices.data(), vertSize);
            [frameRes.vertexBuffer didModifyRange:NSMakeRange(0, vertSize)];
        }
    }
}

void MetalRenderer::updateUniformData(const MetalResources::FrameResources& frameRes)
{
    CameraUtils::MatrixSet matrices = CameraUtils::computeMatrices(m_camera, m_windowInfo.surface_width, m_windowInfo.surface_height);
    UniformBufferUtils::UniformBufferData ubo = UniformBufferUtils::buildUniformBufferData(matrices, m_lighting, true);
    
    memcpy(frameRes.uniformBuffer.contents, &ubo, sizeof(ubo));
    [frameRes.uniformBuffer didModifyRange:NSMakeRange(0, sizeof(ubo))];
}

void MetalRenderer::resize(uint32_t width, uint32_t height)
{
    m_windowInfo.surface_width = width;
    m_windowInfo.surface_height = height;
    if (m_metalLayer) {
        m_metalLayer.drawableSize = CGSizeMake(width, height);
    }
    // Resize depth texture
    m_resources.createDepthTexture(m_device.getDevice(), width, height);
}

void MetalRenderer::setRotation(float x, float y, float z) { m_camera.setRotation(x, y, z); }
void MetalRenderer::setZoom(float zoom) { m_camera.setZoom(zoom); }
void MetalRenderer::setCameraMode(CameraMode mode) {
    m_camera.mode = mode;
    m_camera.applyMode();
}
void MetalRenderer::setLightingFromIconSys(PS2IconSys* iconSys) { m_lighting.loadFromIconSys(iconSys); }
void MetalRenderer::setLightingMode(LightingMode mode) {
    m_lighting.mode = mode;
    m_lighting.applyMode();
}
void MetalRenderer::setBackgroundFromIconSys(PS2IconSys* iconSys) { m_background.loadFromIconSys(iconSys); }
void MetalRenderer::setBackgroundColor(float r, float g, float b, float a) { m_background.setColor(r, g, b, a); }
