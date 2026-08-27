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

bool MetalDevice::initialize()
{
	m_error.Clear();

	m_device = MTLCreateSystemDefaultDevice();
	if (!m_device)
		return m_error.Fail("MTL: Failed to create Metal device");

	m_commandQueue = [m_device newCommandQueue];
	if (!m_commandQueue)
		return m_error.Fail("MTL: Failed to create Metal command queue");

	Logger::info("MTL: Using device: {}", [m_device.name UTF8String]);
	return true;
}

void MetalDevice::shutdown()
{
	m_commandQueue = nil;
	m_device = nil;
}
