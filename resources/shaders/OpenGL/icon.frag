// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#version 330 core

in VS_OUT {
    vec2 texCoord;
    noperspective vec4 color;
} fs_in;

uniform sampler2D texSampler;

uniform int useTexture;
uniform int enableAlpha;
uniform float alphaOverride;

out vec4 outColor;

void main() {
    vec4 texColor = (useTexture != 0) ? texture(texSampler, fs_in.texCoord) : vec4(1.0);
    vec3 color = (useTexture != 0)
        ? floor(texColor.rgb * 255.0 * fs_in.color.rgb / 128.0 + 0.0001) / 255.0
        : floor(fs_in.color.rgb + 0.0001) / 255.0;

    float alpha = (enableAlpha == 1) ? fs_in.color.a : 1.0;
    outColor = vec4(color, alpha);
}
