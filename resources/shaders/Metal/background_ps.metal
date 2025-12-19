// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include <metal_stdlib>
using namespace metal;

struct BGVertexOutput
{
    float4 position [[position]];
    float4 color;
};

fragment float4 PSMain(BGVertexOutput in [[stage_in]])
{
    return in.color;
}
