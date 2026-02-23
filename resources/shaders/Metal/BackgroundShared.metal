// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include <metal_stdlib>
using namespace metal;

struct BGVertexInput
{
    float2 position [[attribute(0)]];
    float4 color    [[attribute(1)]];
};

struct BGVertexOutput
{
    float4 position [[position]];
    float4 color;
};
