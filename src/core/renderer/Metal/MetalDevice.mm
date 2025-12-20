// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MetalDevice.h"
#include "Logger.h"

MetalDevice::MetalDevice()
{
}

MetalDevice::~MetalDevice()
{
    shutdown();
}

bool MetalDevice::initialize()
{
    m_device = MTLCreateSystemDefaultDevice();
    if (!m_device) {
        Logger::error("MTL: Failed to create Metal device");
        return false;
    }
    
    m_commandQueue = [m_device newCommandQueue];
    if (!m_commandQueue) {
        Logger::error("MTL: Failed to create Metal command queue");
        return false;
    }
    
    return true;
}

void MetalDevice::shutdown()
{
    m_commandQueue = nil;
    m_device = nil;
}
