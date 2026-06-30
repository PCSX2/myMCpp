// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <Metal/Metal.h>
#include "../../../common/Error.h"

class MetalDevice
{
public:
	MetalDevice();
	~MetalDevice();

	bool initialize();
	void shutdown();

	const Error& GetError() const { return m_error; }

	id<MTLDevice> getDevice() const { return m_device; }
	id<MTLCommandQueue> getCommandQueue() const { return m_commandQueue; }

private:
	Error m_error;
	id<MTLDevice> m_device = nil;
	id<MTLCommandQueue> m_commandQueue = nil;
};
