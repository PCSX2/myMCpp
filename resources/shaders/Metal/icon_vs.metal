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
    out.position = scene.projection * scene.view * worldPos;

    float3x3 modelMat3x3 = float3x3(
        scene.model[0].xyz,
        scene.model[1].xyz,
        scene.model[2].xyz
    );
    // Reverse rotation for PS2 lighting.
    float3x3 normalMatrix = transpose(modelMat3x3);
    
    float3 normal =
        (in.normal / 4096.0f) * float3(1.0, -1.0, -1.0);

    normal = normalize(normalMatrix * normal);

    out.fragTexCoord = in.texCoord / 4096.0f;
    float3 light = scene.ambientLight.rgb;
    for (uint i = 0; i < 3; i++)
    {
        float diffuse = max(dot(normalize(scene.lightPos[i].xyz), normal), 0.0f);
        light += diffuse * scene.lightColor[i].rgb;
    }
    out.fragColor = float4(floor(clamp(in.color.rgb * 255.0f * light, 0.0f, 255.0f)), in.color.a);

    return out;
}

