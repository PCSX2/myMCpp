// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#pragma once

#include <Metal/Metal.h>

class MetalDevice
{
public:
    MetalDevice();
    ~MetalDevice();
    
    bool initialize();
    void shutdown();
    
    id<MTLDevice> getDevice() const { return m_device; }
    id<MTLCommandQueue> getCommandQueue() const { return m_commandQueue; }
    
private:
    id<MTLDevice> m_device = nil;
    id<MTLCommandQueue> m_commandQueue = nil;
};
};
