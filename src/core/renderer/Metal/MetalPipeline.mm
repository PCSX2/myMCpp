// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MetalPipeline.h"
#include "Logger.h"
#include "ResourcePath.h"

MetalPipeline::MetalPipeline()
{
}

MetalPipeline::~MetalPipeline()
{
	shutdown();
}

bool MetalPipeline::initialize(id<MTLDevice> device)
{
	fs::path resourcePath = ResourcePath::get();
	fs::path metallibPath = resourcePath / "shaders" / "Metal" / "default.metallib";

	NSError* error = nil;
	NSString* path = [NSString stringWithUTF8String:metallibPath.string().c_str()];
	NSURL* url = [NSURL fileURLWithPath:path];
	id<MTLLibrary> library = [device newLibraryWithURL:url error:&error];

	if (!library)
	{
		Logger::error("MTL: Failed to load metallib from {}: {}",
			metallibPath.string(),
			[[error description] UTF8String]);
		return false;
	}

	id<MTLFunction> vertexFunc = [library newFunctionWithName:@"IconVSMain"];
	id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"IconPSMain"];

	if (!vertexFunc || !fragmentFunc)
	{
		Logger::error("MTL: Failed to find shader entry points IconVSMain/IconPSMain");
		return false;
	}

	MTLRenderPipelineDescriptor* psoDesc = [[MTLRenderPipelineDescriptor alloc] init];
	psoDesc.vertexFunction = vertexFunc;
	psoDesc.fragmentFunction = fragmentFunc;
	psoDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
	psoDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

	psoDesc.colorAttachments[0].blendingEnabled = YES;
	psoDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
	psoDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	psoDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
	psoDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
	psoDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	psoDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;

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

	vertexDesc.layouts[2].stride = 36;
	vertexDesc.layouts[2].stepRate = 1;
	vertexDesc.layouts[2].stepFunction = MTLVertexStepFunctionPerVertex;

	psoDesc.vertexDescriptor = vertexDesc;

	m_pipelineState = [device newRenderPipelineStateWithDescriptor:psoDesc error:&error];
	if (!m_pipelineState)
	{
		Logger::error("MTL: Failed to create pipeline state: {}", [[error description] UTF8String]);
		return false;
	}

	MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
	depthDesc.depthCompareFunction = MTLCompareFunctionLess;
	depthDesc.depthWriteEnabled = YES;
	m_depthStencilState = [device newDepthStencilStateWithDescriptor:depthDesc];

	m_library = library;

	return true;
}

bool MetalPipeline::createBackgroundPipeline(id<MTLDevice> device)
{
	if (!m_library)
	{
		Logger::error("MTL: No library loaded - call initialize first");
		return false;
	}

	id<MTLFunction> vertexFunc = [m_library newFunctionWithName:@"BackgroundVSMain"];
	id<MTLFunction> fragmentFunc = [m_library newFunctionWithName:@"BackgroundPSMain"];

	if (!vertexFunc || !fragmentFunc)
	{
		Logger::error("MTL: Failed to find background shader entry points");
		return false;
	}

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

	NSError* error = nil;
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
	m_library = nil;
}
