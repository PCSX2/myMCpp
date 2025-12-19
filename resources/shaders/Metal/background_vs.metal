// SPDX-FileCopyrightText: 2025 SternXD
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

vertex BGVertexOutput VSMain(BGVertexInput in [[stage_in]])
{
    BGVertexOutput out;
    out.position = float4(in.position, 1.0, 1.0); // Z=1.0 (max depth/back), W=1.0
    out.color = in.color;
    return out;
}
