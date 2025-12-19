// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+
// Not used yet

struct VSInput
{
    float3 inPosition : POSITION;
    float3 inNormal   : NORMAL;
    float2 inTexCoord : TEXCOORD0;
    float4 inColor    : COLOR0;
};

struct VSOutput
{
    float3 fragNormal    : TEXCOORD0;
    float2 fragTexCoord  : TEXCOORD1;
    float4 fragColor     : TEXCOORD2;
    float3 fragWorldPos  : TEXCOORD3;
    float4 position      : SV_Position;
};

// Root CBV (b0)
cbuffer UniformBufferObject : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;

    float4 lightPos[3];
    float4 lightColor[3];
    float4 ambientLight;
    float4 time;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Fixed-point -> float + PS2 → GL/VK orientation fix
    float3 position =
        (input.inPosition / 4096.0f) * float3(1.0f, -1.0f, -1.0f);

    // World transform
    float4 worldPos = mul(float4(position, 1.0f), model);
    output.fragWorldPos = worldPos.xyz;

    // Clip-space transform
    output.position = mul(worldPos, view);
    output.position = mul(output.position, projection);

    // Normal matrix (transpose(inverse(model)))
    float3x3 normalMatrix = transpose((float3x3)inverse(model));
    float3 normal =
        (input.inNormal / 4096.0f) * float3(1.0f, -1.0f, -1.0f);

    output.fragNormal = normalize(mul(normal, normalMatrix));

    // PS2 fixed-point UVs
    output.fragTexCoord = input.inTexCoord / 4096.0f;

    output.fragColor = input.inColor;

    return output;
}
