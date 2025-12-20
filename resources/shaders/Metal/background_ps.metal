// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "BackgroundShared.metal"

fragment float4 BackgroundPSMain(BGVertexOutput in [[stage_in]])
{
    return in.color;
}
