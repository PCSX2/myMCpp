// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 0) out vec4 vertColor;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    vertColor = inColor;
}
