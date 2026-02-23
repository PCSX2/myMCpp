// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "IconShared.metal"

vertex VSOutput IconVSMain(
    VSInput in               [[stage_in]],
    constant SceneCB& scene  [[buffer(0)]]
)
{
    VSOutput out;

    float3 position =
        (in.position / 4096.0f) * float3(1.0, -1.0, -1.0);

    float4 worldPos = scene.model * float4(position, 1.0);
    out.fragWorldPos = worldPos.xyz;

    out.position = scene.projection * scene.view * worldPos;

    float3x3 modelMat3x3 = float3x3(
        scene.model[0].xyz,
        scene.model[1].xyz,
        scene.model[2].xyz
    );
    float3x3 normalMatrix = transpose(modelMat3x3);
    
    float3 normal =
        (in.normal / 4096.0f) * float3(1.0, -1.0, -1.0);

    out.fragNormal = normalize(normalMatrix * normal);

    out.fragTexCoord = in.texCoord / 4096.0f;
    out.fragColor    = in.color;

    return out;
}

