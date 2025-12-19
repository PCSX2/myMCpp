// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 450

// Input from vertex shader
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec3 fragWorldPos;

// Uniform buffer (shared with vertex shader)
// std140-friendly: pad vec3 to vec4 to match host layout
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos[3];       // 3 light sources
    vec4 lightColor[3];
    vec4 ambientLight;
    vec4 time;
} ubo;

// Texture sampler
layout(binding = 1) uniform sampler2D texSampler;

// Push constants (kept for interface; only use texture flag)
layout(push_constant) uniform PushConstants {
    int useTexture;         // 1 = use texture, 0 = vertex colors only
    int enableAlpha;        // 1 = enable alpha blending (unused in shader)
    float alphaOverride;    // Unused
} pc;

// Output
layout(location = 0) out vec4 outColor;

void main() {
    // PS2 uses 0-128 color range where 128 = full intensity
    // After normalization (divide by 255), 128 becomes ~0.5, so multiply by 2.0
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 vertColor = fragColor.rgb * 2.0;
    vec3 baseColor = vertColor * texColor.rgb;

    // Lighting: ambient + diffuse
    vec3 normal = normalize(fragNormal);
    vec3 color = baseColor * ubo.ambientLight.rgb;
    
    for (int i = 0; i < 3; i++) {
        vec3 lightDir = normalize(ubo.lightPos[i].xyz);
        float lambert = max(dot(lightDir, normal), 0.0);
        color += lambert * ubo.lightColor[i].rgb * baseColor;
    }

    // When enableAlpha=0 (all vertices have alpha=0), use 1.0 to make icon visible
    // When enableAlpha=1 (some vertices have alpha>0), use vertex alpha for transparency
    float alpha = (pc.enableAlpha == 1) ? fragColor.a : 1.0;
    outColor = vec4(color, alpha);
}
