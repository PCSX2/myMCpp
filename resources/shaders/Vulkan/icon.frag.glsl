// SPDX-FileCopyrightText: 2025 SternXD
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
    vec4 lightPos[3];       // 3 light sources
    vec4 lightColor[3];
    vec4 ambientLight;
    vec4 time;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    int useTexture;         // 1 = use texture, 0 = vertex colors only
    int enableAlpha;        // 1 = enable alpha blending (unused in shader)
    float alphaOverride;    // Unused
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 vertColor = fragColor.rgb * 2.0;
    vec3 baseColor = vertColor * texColor.rgb;

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
