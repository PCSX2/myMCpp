// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MetalPipeline.h"
#include "../../common/Logger.h"
#include "../../common/ResourcePath.h"
#include <fstream>
#include <sstream>

MetalPipeline::MetalPipeline()
{
}

MetalPipeline::~MetalPipeline()
{
    shutdown();
}

static std::string LoadShaderFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool MetalPipeline::initialize(id<MTLDevice> device)
{
    fs::path resourcePath = ResourcePath::get();
    
    // Load shader sources
    std::string sharedSrc = LoadShaderFile((resourcePath / "shaders" / "Metal" / "IconShared.metal").string());
    std::string vsSrc = LoadShaderFile((resourcePath / "shaders" / "Metal" / "icon_vs.metal").string());
    std::string psSrc = LoadShaderFile((resourcePath / "shaders" / "Metal" / "icon_ps.metal").string());

    if (sharedSrc.empty() || vsSrc.empty() || psSrc.empty()) {
        Logger::error("MTL: Failed to load Metal shader files from {}", (resourcePath / "shaders" / "Metal").string());
        return false;
    }
    
    auto StripInclude = [](std::string& src) {
        std::string search = "#include \"IconShared.metal\"";
        size_t pos = src.find(search);
        if (pos != std::string::npos) {
            src.replace(pos, search.length(), "");
        }
    };
    
    StripInclude(vsSrc);
    StripInclude(psSrc);
    
    std::string fullSource = sharedSrc + "\n" + vsSrc + "\n" + psSrc;

    NSError* error = nil;
    NSString* shaderSrc = [NSString stringWithUTF8String:fullSource.c_str()];
    id<MTLLibrary> library = [device newLibraryWithSource:shaderSrc options:nil error:&error];
    if (!library) {
        Logger::error("MTL: Failed to compile Metal shaders: {}", [[error description] UTF8String]);
        return false;
    }
    
    id<MTLFunction> vertexFunc = [library newFunctionWithName:@"VSMain"];
    id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"PSMain"];
    
    if (!vertexFunc || !fragmentFunc) {
        Logger::error("MTL: Failed to find shader entry points VSMain/PSMain");
        return false;
    }

    MTLRenderPipelineDescriptor* psoDesc = [[MTLRenderPipelineDescriptor alloc] init];
    psoDesc.vertexFunction = vertexFunc;
    psoDesc.fragmentFunction = fragmentFunc;
    psoDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    psoDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    
    MTLVertexDescriptor* vertexDesc = [[MTLVertexDescriptor alloc] init];
    
    vertexDesc.attributes[0].format = MTLVertexFormatFloat3; // Pos
    vertexDesc.attributes[0].offset = 0;
    vertexDesc.attributes[0].bufferIndex = 2; // Buffer 2 to avoid conflict with SceneCB(0) and PushCB(1)
    
    vertexDesc.attributes[1].format = MTLVertexFormatFloat3; // Norm
    vertexDesc.attributes[1].offset = 12;
    vertexDesc.attributes[1].bufferIndex = 2;
    
    vertexDesc.attributes[2].format = MTLVertexFormatFloat2; // UV
    vertexDesc.attributes[2].offset = 24;
    vertexDesc.attributes[2].bufferIndex = 2;
    
    vertexDesc.attributes[3].format = MTLVertexFormatUChar4Normalized; // Color
    vertexDesc.attributes[3].offset = 32;
    vertexDesc.attributes[3].bufferIndex = 2;
    
    vertexDesc.layouts[0].stride = 36;
    vertexDesc.layouts[0].stepRate = 1;
    vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    psoDesc.vertexDescriptor = vertexDesc;
    
    m_pipelineState = [device newRenderPipelineStateWithDescriptor:psoDesc error:&error];
    if (!m_pipelineState) {
        Logger::error("MTL: Failed to create pipeline state: {}", [[error description] UTF8String]);
        return false;
    }
    
    MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthDesc.depthWriteEnabled = YES;
    m_depthStencilState = [device newDepthStencilStateWithDescriptor:depthDesc];
    
    return true;
}

bool MetalPipeline::createBackgroundPipeline(id<MTLDevice> device)
{
    fs::path resourcePath = ResourcePath::get();
    
    std::string vsSrc = LoadShaderFile((resourcePath / "shaders" / "Metal" / "background_vs.metal").string());
    std::string psSrc = LoadShaderFile((resourcePath / "shaders" / "Metal" / "background_ps.metal").string());
    
    if (vsSrc.empty() || psSrc.empty()) {
        Logger::error("MTL: Failed to load Metal background shader files");
        return false;
    }
    
    std::string fullSource = vsSrc + "\n" + psSrc;
    
    NSError* error = nil;
    NSString* shaderSrc = [NSString stringWithUTF8String:fullSource.c_str()];
    id<MTLLibrary> library = [device newLibraryWithSource:shaderSrc options:nil error:&error];
    if (!library) {
        Logger::error("MTL: Failed to compile Metal background shaders: {}", [[error description] UTF8String]);
        return false;
    }
    
    id<MTLFunction> vertexFunc = [library newFunctionWithName:@"VSMain"];
    id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"PSMain"];
    
    if (!vertexFunc || !fragmentFunc) return false;
    
    MTLRenderPipelineDescriptor* psoDesc = [[MTLRenderPipelineDescriptor alloc] init];
    psoDesc.vertexFunction = vertexFunc;
    psoDesc.fragmentFunction = fragmentFunc;
    psoDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    psoDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    
    MTLVertexDescriptor* vertexDesc = [[MTLVertexDescriptor alloc] init];
    
    vertexDesc.attributes[0].format = MTLVertexFormatFloat2; // Pos
    vertexDesc.attributes[0].offset = 0;
    vertexDesc.attributes[0].bufferIndex = 0;
    
    vertexDesc.attributes[1].format = MTLVertexFormatUChar4Normalized; // Color
    vertexDesc.attributes[1].offset = 8;
    vertexDesc.attributes[1].bufferIndex = 0;
    
    vertexDesc.layouts[0].stride = 12;
    vertexDesc.layouts[0].stepRate = 1;
    vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    psoDesc.vertexDescriptor = vertexDesc;
    
    m_bgPipelineState = [device newRenderPipelineStateWithDescriptor:psoDesc error:&error];
    
    MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthDesc.depthCompareFunction = MTLCompareFunctionAlways;
    depthDesc.depthWriteEnabled = NO;
    m_bgDepthStencilState = [device newDepthStencilStateWithDescriptor:depthDesc];
    
    return m_bgPipelineState != nil;
}

void MetalPipeline::shutdown()
{
    m_pipelineState = nil;
    m_bgPipelineState = nil;
    m_depthStencilState = nil;
    m_bgDepthStencilState = nil;
}
