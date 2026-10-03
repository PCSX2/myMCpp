// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 450

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) noperspective in vec4 fragColor;

layout(binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    int useTexture;
    int enableAlpha;
    float alphaOverride;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = (pc.useTexture != 0) ? texture(texSampler, fragTexCoord) : vec4(1.0);
    vec3 color = (pc.useTexture != 0)
        ? floor(texColor.rgb * 255.0 * fragColor.rgb / 128.0 + 0.0001) / 255.0
        : floor(fragColor.rgb + 0.0001) / 255.0;

    float alpha = (pc.enableAlpha == 1) ? fragColor.a : 1.0;
    outColor = vec4(color, alpha);
}
