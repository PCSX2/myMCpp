// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "IconShared.metal"

fragment float4 IconPSMain(
    VSOutput in                      [[stage_in]],
    constant PushCB& push            [[buffer(1)]],
    texture2d<float> tex             [[texture(0)]],
    sampler texSampler               [[sampler(0)]]
)
{
    float4 texColor = (push.useTexture != 0)
        ? tex.sample(texSampler, in.fragTexCoord)
        : float4(1.0);

    float3 color = (push.useTexture != 0)
        ? floor(texColor.rgb * 255.0f * in.fragColor.rgb / 128.0f + 0.0001f) / 255.0f
        : floor(in.fragColor.rgb + 0.0001f) / 255.0f;

    float alpha = (push.enableAlpha != 0) ? in.fragColor.a : push.alphaOverride;

    return float4(color, alpha);
}
