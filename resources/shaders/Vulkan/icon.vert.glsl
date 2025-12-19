// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 450

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;

// Uniform buffer (Model-View-Projection matrices)
// std140-friendly: pad vec3 to vec4 to match host layout
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 lightPos[3];       // 3 light sources
    vec4 lightColor[3];
    vec4 ambientLight;
    vec4 time;              // x = time, others padding
} ubo;

// Output to fragment shader
layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out vec3 fragWorldPos;

void main() {
    // Convert fixed-point position to float and match PS2 -> GL/VK orientation
    vec3 position = (inPosition / 4096.0) * vec3(1.0, -1.0, -1.0);
    
    // Transform position
    vec4 worldPos = ubo.model * vec4(position, 1.0);
    fragWorldPos = worldPos.xyz;
    
    gl_Position = ubo.projection * ubo.view * worldPos;
    
    // Transform normal to world space
    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
    fragNormal = normalize(normalMatrix * ((inNormal / 4096.0) * vec3(1.0, -1.0, -1.0)));
    
    // PS2 UVs are fixed-point; divide by 4096
    fragTexCoord = inTexCoord / 4096.0;
    fragColor = inColor;
}
