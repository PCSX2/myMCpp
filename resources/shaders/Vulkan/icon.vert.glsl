// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos[3];
    vec4 lightColor[3];
    vec4 ambientLight;
    vec4 time;
} ubo;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) noperspective out vec4 fragColor;

void main() {
    vec3 position = (inPosition / 4096.0) * vec3(1.0, -1.0, -1.0);

    vec4 worldPos = ubo.model * vec4(position, 1.0);
    gl_Position = ubo.projection * ubo.view * worldPos;

    // Reverse rotation for PS2 lighting.
    mat3 normalMatrix = transpose(mat3(ubo.model));
    vec3 normal = normalize(normalMatrix * ((inNormal / 4096.0) * vec3(1.0, -1.0, -1.0)));

    fragTexCoord = inTexCoord / 4096.0;
    vec3 light = ubo.ambientLight.rgb;
    for (int i = 0; i < 3; i++) {
        float diffuse = max(dot(normalize(ubo.lightPos[i].xyz), normal), 0.0);
        light += diffuse * ubo.lightColor[i].rgb;
    }
    fragColor = vec4(floor(clamp(inColor.rgb * 255.0 * light, 0.0, 255.0)), inColor.a);
}
