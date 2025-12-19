// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "IconShared.metal"

fragment float4 PSMain(
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

    float3 baseColor = in.fragColor.rgb * texColor.rgb;

    float3 normal = normalize(in.fragNormal);
    float3 color  = baseColor * scene.ambientLight.rgb;

    for (uint i = 0; i < 3; i++)
    {
        float3 lightDir = normalize(scene.lightPos[i].xyz);
        float lambert   = max(dot(lightDir, normal), 0.0);
        color += lambert * scene.lightColor[i].rgb * baseColor;
    }

    float alpha = in.fragColor.a;

    if (push.enableAlpha != 0)
    {
        alpha = push.alphaOverride;
    }

    return float4(color, alpha);
}
