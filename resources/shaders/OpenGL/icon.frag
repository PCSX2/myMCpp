// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 330 core

// Input from vertex shader
in VS_OUT {
    vec3 normal;
    vec2 texCoord;
    vec4 color;
    vec3 worldPos;
} fs_in;

// Uniform buffer object (emulates UBO binding 0)
// std140-friendly: pad vec3 to vec4 to match host layout
uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos[3];       // 3 light sources
    vec4 lightColor[3];
    vec4 ambientLight;
    vec4 time;
} ubo;

// Texture sampler
uniform sampler2D texSampler;

// Rendering options (keep for parity but keep behavior simple)
uniform int useTexture;         // 1 = use texture, 0 = vertex colors only
uniform int enableAlpha;        // 1 = enable alpha blending (unused in shader, used in pipeline)
uniform float alphaOverride;    // Unused (legacy)

// Output
out vec4 outColor;

void main() {
    // PS2 uses 0-128 color range where 128 = full intensity
    // After normalization (divide by 255), 128 becomes ~0.5, so multiply by 2.0
    vec4 texColor = texture(texSampler, fs_in.texCoord);
    vec3 vertColor = fs_in.color.rgb * 2.0;
    vec3 baseColor = vertColor * texColor.rgb;

    // Lighting: ambient + diffuse
    vec3 normal = normalize(fs_in.normal);
    vec3 color = baseColor * ubo.ambientLight.rgb;
    
    for (int i = 0; i < 3; i++) {
        vec3 lightDir = normalize(ubo.lightPos[i].xyz);
        float lambert = max(dot(lightDir, normal), 0.0);
        color += lambert * ubo.lightColor[i].rgb * baseColor;
    }

    // When enableAlpha=0 (all vertices have alpha=0), use 1.0 to make icon visible
    // When enableAlpha=1 (some vertices have alpha>0), use vertex alpha for transparency
    float alpha = (enableAlpha == 1) ? fs_in.color.a : 1.0;
    outColor = vec4(color, alpha);
}
