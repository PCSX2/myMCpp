// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 330 core

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;

// Uniform buffer object (emulates UBO binding 0)
// std140-friendly: pad vec3 to vec4 to keep host writes simple
uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos[3];       // 3 light sources
    vec4 lightColor[3];
    vec4 ambientLight;
    vec4 time;              // x = time, others padding
} ubo;

// Output to fragment shader
out VS_OUT {
    vec3 normal;
    vec2 texCoord;
    vec4 color;
    vec3 worldPos;
} vs_out;

void main() {
    // Convert fixed-point position to float and match PS2 -> GL orientation
    vec3 position = (inPosition / 4096.0) * vec3(1.0, -1.0, -1.0);
    
    // Transform position
    vec4 worldPos = ubo.model * vec4(position, 1.0);
    vs_out.worldPos = worldPos.xyz;
    
    gl_Position = ubo.projection * ubo.view * worldPos;
    
    // Transform normal to world space
    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
    vs_out.normal = normalize(normalMatrix * ((inNormal / 4096.0) * vec3(1.0, -1.0, -1.0)));
    
    // PS2 UVs are fixed-point; divide by 4096
    vs_out.texCoord = inTexCoord / 4096.0;
    vs_out.color = inColor;
}
