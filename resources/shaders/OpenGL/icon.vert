// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 330 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;

uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos[3];
    vec4 lightColor[3];
    vec4 ambientLight;
    vec4 time;
} ubo;

out VS_OUT {
    vec3 normal;
    vec2 texCoord;
    vec4 color;
    vec3 worldPos;
} vs_out;

void main() {
    vec3 position = (inPosition / 4096.0) * vec3(1.0, -1.0, -1.0);

    vec4 worldPos = ubo.model * vec4(position, 1.0);
    vs_out.worldPos = worldPos.xyz;

    gl_Position = ubo.projection * ubo.view * worldPos;

    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
    vs_out.normal = normalize(normalMatrix * ((inNormal / 4096.0) * vec3(1.0, -1.0, -1.0)));

    vs_out.texCoord = inTexCoord / 4096.0;
    vs_out.color = inColor;
}
