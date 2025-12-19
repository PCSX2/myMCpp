// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+
// Not used yet

struct PSInput
{
    float3 fragNormal    : TEXCOORD0;
    float2 fragTexCoord  : TEXCOORD1;
    float4 fragColor     : TEXCOORD2;
    float3 fragWorldPos  : TEXCOORD3; // unused
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

// Push constants equivalent (b1)
cbuffer PushConstants : register(b1)
{
    int   useTexture;
    int   enableAlpha;
    float alphaOverride;
    float _pad; // 16-byte alignment
};

// Descriptor table (t0 / s0)
Texture2D    texSampler : register(t0);
SamplerState texState   : register(s0);

float4 main(PSInput input) : SV_Target
{
    float4 texColor  = texSampler.Sample(texState, input.fragTexCoord);
    float3 baseColor = input.fragColor.rgb * texColor.rgb;

    // Ambient + diffuse lighting
    float3 normal = normalize(input.fragNormal);
    float3 color  = baseColor * ambientLight.rgb;

    [unroll]
    for (int i = 0; i < 3; i++)
    {
        float3 lightDir = normalize(lightPos[i].xyz);
        float lambert   = max(dot(lightDir, normal), 0.0);
        color += lambert * lightColor[i].rgb * baseColor;
    }

    // Vertex alpha only
    return float4(color, input.fragColor.a);
}
