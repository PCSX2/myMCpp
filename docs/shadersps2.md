# OpenGL Shader Rendering in ps2mc-browser

<!-- https://babyno.top/en/posts/2023/12/ps2mc-browsers-shader-introduction/ -->

---

## Rendering Vertices, Polygons, and Textures

How do we render the vertices and textures of polygons into a colorful scene?  
This is where **OpenGL shaders** come into play.

Today, we'll discuss the shaders used in **ps2mc-browser**.

**ps2mc-browser** is a PlayStation 2 memory card viewer capable of parsing **vertex and texture data** from 3D icons stored in PS2 memory card files and rendering them using **OpenGL**.

In the following sections, we'll break down the **six OpenGL shaders** used in ps2mc-browser and explain how each one works.

---

## Background Shaders (`bg.frag` and `bg.vert`)

These shaders are responsible for rendering the **background color**.

From earlier analysis, the `icon.sys` file provides:
- Color data
- Transparency data
for the **four vertices** of the background.

### Coordinate System Recap

- The scene is a **cube with side length = 2**
- The origin `(0, 0, 0)` is at the **center**
- The camera looks down the **negative Z-axis**

The background face is placed slightly in front of the camera:

```glsl
bg_vertex = [
    (-1,  1, 0.99),
    (-1, -1, 0.99),
    ( 1, -1, 0.99),
    ( 1,  1, 0.99)
]

Because shaders render triangles, this quad is split into **two triangles**.
Each vertex is assigned a color, and OpenGL interpolates the colors across the face.

### Fragment Shader: bg.frag

```glsl
#version 330 core
in vec4 fragColor0;
out vec4 fragColor;

void main() {
    fragColor = fragColor0;
}
```

This shader simply outputs the interpolated color passed from the vertex shader.

### Vertex Shader: bg.vert

```glsl
#version 330 core
in vec3 vertexPos;
in vec4 vertexColor;

out vec4 fragColor0;

void main() {
    fragColor0 = vertexColor;
    gl_Position = vec4(vertexPos, 1.0);
}
```

* Receives vertex position and color
* Passes color to the fragment shader
* Sets the final clip-space position

**Purpose:** Fill the screen with the desired background color

### Transparency and the Skybox Layer

The background also supports alpha transparency.
To visualize transparency, a skybox layer is rendered behind the background.

```glsl
skybox_vertex = [
    (-1,  1, 0.999),
    (-1, -1, 0.999),
    ( 1, -1, 0.999),
    ( 1,  1, 0.999)
]

skybox_colors = [
    (0.6, 0.6, 0.6, 1),
    (0.6, 0.6, 0.6, 1),
    (0.6, 0.6, 0.6, 1),
    (0.6, 0.6, 0.6, 1)
]
```

If the background has transparency, the skybox color shows through, producing the smooth color blending effect seen in the UI.

---

## Icon Shaders (`icon.frag` and `icon.vert`)

These shaders handle the 3D icons extracted from PS2 memory cards.
They are the most complex shaders in the project.

### Fragment Shader: icon.frag

```glsl
#version 330 core
uniform sampler2D texture0;
uniform vec4 ambient;
uniform mat4 model;
uniform Light lights[MAX_NUM_TOTAL_LIGHTS];

void main() {
    vec3 normal = normalize(normal0).xyz;
    vec3 color = texture(texture0, uv0).rgb;

    vec3 diffuse = vec3(0);
    for (int i = 0; i < MAX_NUM_TOTAL_LIGHTS; i++) {
        vec3 lightDir = normalize(lights[i].dir.xyz);
        diffuse += max(dot(normal, lightDir), 0.0) * lights[i].color.rgb;
    }

    vec4 finalColor = vec4((ambient.rgb + diffuse) * color, 1.0);
    fragColor = finalColor;
}
```

**What this does:**

* Samples the texture color
* Computes per-pixel diffuse lighting
* Adds ambient lighting
* Outputs the final shaded color

### Vertex Shader: icon.vert

```glsl
#version 330 core
out vec2 uv0;
out vec4 normal0;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;
uniform float tweenFactor;

void main() {
    uv0 = texCoord;
    normal0 = model * vec4(normal, 1);

    vec4 basePos = vec4(
        mix(vertexPos, nextVertexPos, tweenFactor),
        1.0
    );

    gl_Position = proj * view * model * basePos;
}
```

**Key Concept: Vertex Animation**

PS2 icons store multiple animation frames.
To animate smoothly, vertices are interpolated between frames.
* tweenFactor controls interpolation
* mix() blends current and next frame vertices
* Produces smooth animated motion

### Button Shaders (circle.frag and circle.vert)

These shaders render interactive buttons used to switch animation states.

#### Fragment Shader: circle.frag

```glsl
#version 330 core
out vec4 fragColor;

void main() {
    fragColor = vec4(1.0, 1.0, 1.0, 0.6);
}
```

**Outputs semi-transparent white**

**No inputs required**

#### Vertex Shader: circle.vert

```glsl
#version 330 core
in vec2 vertexPos;

void main() {
    gl_Position = vec4(vertexPos, 0, 1.0);
}
```

**Positions 2D geometry directly in clip space**

**Purpose:** Render simple translucent shapes used as clickable UI elements.

### Summary

ps2mc-browser renders animated PS2 memory card icons by combining:

- Background shaders (color + transparency)
- Icon shaders (texturing, lighting, animation)
- Button shaders (UI interaction)

Both Python and OpenGL were new technologies for this project, and integrating them proved to be a rewarding challenge. More features may be added in the future.
