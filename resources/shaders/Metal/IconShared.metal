// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include <metal_stdlib>
using namespace metal;

struct SceneCB
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;

    float4 lightPos[3];
    float4 lightColor[3];
    float4 ambientLight;
    float4 time;
};

struct PushCB
{
    int   useTexture;      // 1 = use texture
    int   enableAlpha;     // 1 = override alpha
    float alphaOverride;  // override value
    float _pad;            // 16-byte alignment
};

struct VSInput
{
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 texCoord [[attribute(2)]];
    float4 color    [[attribute(3)]];
};

struct VSOutput
{
    float4 position     [[position]];
    float3 fragNormal;
    float2 fragTexCoord;
    float4 fragColor;
    float3 fragWorldPos;
};
