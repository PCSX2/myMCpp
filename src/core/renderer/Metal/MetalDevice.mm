// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "MetalDevice.h"
#include "common/Logger.h"

MetalDevice::MetalDevice()
{
}

MetalDevice::~MetalDevice()
{
	shutdown();
}

bool MetalDevice::initialize(const std::string& preferredAdapter)
{
	m_error.Clear();

	m_device = MTLCreateSystemDefaultDevice();
	if (!preferredAdapter.empty())
	{
		for (id<MTLDevice> device in MTLCopyAllDevices())
		{
			const char* name = [device.name UTF8String];
			if (name && preferredAdapter == name)
			{
				m_device = device;
				break;
			}
		}
	}

	if (!m_device)
		return m_error.Fail("MTL: Failed to create Metal device");

	m_commandQueue = [m_device newCommandQueue];
	if (!m_commandQueue)
		return m_error.Fail("MTL: Failed to create Metal command queue");

	const char* deviceName = [m_device.name UTF8String];
	Logger::info("MTL: Using device: {}", deviceName ? deviceName : "Unknown");
	return true;
}

void MetalDevice::shutdown()
{
	m_commandQueue = nil;
	m_device = nil;
}
