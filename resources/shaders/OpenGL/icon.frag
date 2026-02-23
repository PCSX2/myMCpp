// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 330 core

in VS_OUT {
    vec3 normal;
    vec2 texCoord;
    vec4 color;
    vec3 worldPos;
} fs_in;

uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos[3];
    vec4 lightColor[3];
    vec4 ambientLight;
    vec4 time;
} ubo;

uniform sampler2D texSampler;

uniform int useTexture;
uniform int enableAlpha;
uniform float alphaOverride;

out vec4 outColor;

void main() {
    vec4 texColor = (useTexture != 0) ? texture(texSampler, fs_in.texCoord) : vec4(1.0);
    vec3 vertColor = fs_in.color.rgb;
    vec3 baseColor = (useTexture != 0) ? (vertColor * texColor.rgb) : vertColor;

    vec3 normal = normalize(fs_in.normal);
    vec3 color = baseColor * ubo.ambientLight.rgb;

    for (int i = 0; i < 3; i++) {
        vec3 lightDir = normalize(ubo.lightPos[i].xyz);
        float lambert = max(dot(lightDir, normal), 0.0);
        color += lambert * ubo.lightColor[i].rgb * baseColor;
    }

    float alpha = (enableAlpha == 1) ? fs_in.color.a : 1.0;
    outColor = vec4(color, alpha);
}
