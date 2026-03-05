// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "IconShared.metal"

fragment float4 IconPSMain(
    VSOutput in                      [[stage_in]],
    constant SceneCB& scene          [[buffer(0)]],
    constant PushCB& push            [[buffer(1)]],
    texture2d<float> tex             [[texture(0)]],
    sampler texSampler               [[sampler(0)]]
)
{
    float4 texColor = (push.useTexture != 0)
        ? tex.sample(texSampler, in.fragTexCoord)
        : float4(1.0);

    float3 baseColor = (push.useTexture != 0) ? (in.fragColor.rgb * texColor.rgb) : in.fragColor.rgb;

    float3 normal = normalize(in.fragNormal);
    float3 color  = baseColor * scene.ambientLight.rgb;

    for (uint i = 0; i < 3; i++)
    {
        float3 lightDir = normalize(scene.lightPos[i].xyz);
        float lambert   = max(dot(lightDir, normal), 0.0);
        color += lambert * scene.lightColor[i].rgb * baseColor;
    }

    float alpha = (push.enableAlpha != 0) ? in.fragColor.a : push.alphaOverride;

    return float4(color, alpha);
}
