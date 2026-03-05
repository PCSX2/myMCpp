// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "BackgroundShared.metal"

vertex BGVertexOutput BackgroundVSMain(BGVertexInput in [[stage_in]])
{
    BGVertexOutput out;
    out.position = float4(in.position, 0.0, 1.0);
    out.color = in.color;
    return out;
}
