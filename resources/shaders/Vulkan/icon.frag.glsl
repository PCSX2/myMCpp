// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec3 fragWorldPos;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos[3];
    vec4 lightColor[3];
    vec4 ambientLight;
    vec4 time;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    int useTexture;
    int enableAlpha;
    float alphaOverride;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = (pc.useTexture != 0) ? texture(texSampler, fragTexCoord) : vec4(1.0);
    vec3 vertColor = fragColor.rgb;
    vec3 baseColor = (pc.useTexture != 0) ? (vertColor * texColor.rgb) : vertColor;

    vec3 normal = normalize(fragNormal);
    vec3 color = baseColor * ubo.ambientLight.rgb;

    for (int i = 0; i < 3; i++) {
        vec3 lightDir = normalize(ubo.lightPos[i].xyz);
        float lambert = max(dot(lightDir, normal), 0.0);
        color += lambert * ubo.lightColor[i].rgb * baseColor;
    }

    float alpha = (pc.enableAlpha == 1) ? fragColor.a : 1.0;
    outColor = vec4(color, alpha);
}
