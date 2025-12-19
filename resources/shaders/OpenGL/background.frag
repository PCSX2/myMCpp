// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 330 core

// Input from vertex shader
in vec4 vertexColor;

// Output
out vec4 outColor;

void main() {
    outColor = vertexColor;
}
