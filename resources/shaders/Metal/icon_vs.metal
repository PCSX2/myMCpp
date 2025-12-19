// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "IconShared.metal"

vertex VSOutput VSMain(
    VSInput in               [[stage_in]],
    constant SceneCB& scene  [[buffer(0)]]
)
{
    VSOutput out;

    // Fixed-point → float + PS2 axis correction
    float3 position =
        (in.position / 4096.0f) * float3(1.0, -1.0, -1.0);

    float4 worldPos = scene.model * float4(position, 1.0);
    out.fragWorldPos = worldPos.xyz;

    out.position = scene.projection * scene.view * worldPos;

    // Normal matrix
    float3x3 normalMatrix = transpose(inverse(float3x3(scene.model)));
    float3 normal =
        (in.normal / 4096.0f) * float3(1.0, -1.0, -1.0);

    out.fragNormal = normalize(normalMatrix * normal);

    // PS2 fixed-point UVs
    out.fragTexCoord = in.texCoord / 4096.0f;
    out.fragColor    = in.color;

    return out;
}
