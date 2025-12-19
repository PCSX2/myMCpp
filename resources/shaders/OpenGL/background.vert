// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 330 core

layout(location = 0) in vec2 inPosition; // NDC position
layout(location = 3) in vec4 inColor; // Color per vertex

out vec4 vertexColor;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    vertexColor = inColor;
}
