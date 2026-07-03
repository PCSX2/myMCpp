### Icon

<!-- https://babyno.top/en/posts/2023/10/parsing-ps2-3d-icon/ -->

Unlike the icon.sys file, the icon file for each game is variable in size and quantity, but there is always at least one. Some games may use the same icon for both copying and deleting icons as the regular icon.

Seeing this image, it should be familiar to seasoned PS2 players.
It is the **3D icon of a game save file** shown in the PS2 memory card
management interface. This article introduces how to **extract and render
the icon model** from a PS2 save file.

---

## 01 Parsing Objectives

### A. What can be parsed from the save file?

- All **vertices and normals** of the icon model
- **Animation frames** of the icon model
- **Lighting information**
- **Textures and texture coordinates**
- **Background color and transparency**

### B. What needs to be implemented?

- Write shaders to render the **background and icons**
- Create animations from the **icon model's animation frames**
- Build **model**, **view**, and **projection** matrices to achieve a result
  close to the original PS2 visual effect

Completing all functionality will likely require two articles.
This article primarily focuses on **Part A (parsing)**.

---

## 02 Parsing `icon.sys`

In the previous article, we discussed how to export game save files.
Each save file contains an `icon.sys` file, which acts as the **configuration
file** for the save icon.

- File size: **964 bytes**
- Fixed layout

### `icon.sys` Structure

| Offset | Length       | Description                                                                 |
|--------|--------------|-----------------------------------------------------------------------------|
| 0      | byte[4]      | magic: `PS2D`                                                               |
| 4      | uint16       | `0`                                                                         |
| 6      | uint16       | Position of newline character in the game title (see Note 1)               |
| 8      | uint32       | `0`                                                                         |
| 12     | uint32       | bg_transparency: Background transparency (0-255)                           |
| 16     | uint32[4]    | bg_color: Top-left corner background color (RGB, 0-255)                    |
| 32     | uint32[4]    | bg_color: Top-right corner background color (RGB, 0-255)                   |
| 48     | uint32[4]    | bg_color: Bottom-left corner background color (RGB, 0-255)                 |
| 64     | uint32[4]    | bg_color: Bottom-right corner background color (RGB, 0-255)                |
| 80     | uint32[4]    | light_pos1: Light position 1 (XYZ, 0-1)                                     |
| 96     | uint32[4]    | light_pos2: Light position 2 (XYZ, 0-1)                                     |
| 112    | uint32[4]    | light_pos3: Light position 3 (XYZ, 0-1)                                     |
| 128    | uint32[4]    | light_color1: Light color 1 (RGB, 0-1)                                      |
| 144    | uint32[4]    | light_color2: Light color 2 (RGB, 0-1)                                      |
| 160    | uint32[4]    | light_color3: Light color 3 (RGB, 0-1)                                      |
| 176    | uint32[4]    | ambient: Ambient light color (RGB, 0-1)                                    |
| 192    | byte[68]     | sub_title: Game title (null-terminated, SJIS encoding)                     |
| 260    | byte[64]     | icon_file_normal: Normal icon filename (null-terminated)                   |
| 324    | byte[64]     | icon_file_copy: Copy icon filename (null-terminated)                       |
| 388    | byte[64]     | icon_file_delete: Delete icon filename (null-terminated)                   |
| 452    | byte[512]    | All zero                                                                    |

---

### Notes

**Note 1**
The game title (`sub_title`) is displayed across **two lines**.
This value specifies the byte position at which the newline occurs in the
title string.

**Note 2**
Icon filenames refer to `.icn` files stored alongside `icon.sys` in the save
directory.

---

## 03 Parsing the Icon File

The game icon file (usually with a `.icn` extension) contains the 3D model, animations, and texture data. It is composed of four main segments:

### 3.1 Icon File Structure

| Name               | Description                                                         |
|--------------------|---------------------------------------------------------------------|
| Icon Header        | Fixed size, 20 bytes                                                 |
| Vertex Segment     | Contains all vertices and normals data of the icon model             |
| Animation Segment  | Stores information about animation frames of the icon model          |
| Texture Segment    | Stores texture data of the icon model                                |

### 3.2 Icon Header
The Icon header stores all the essential information needed to decode the different data segments. This includes:

Number of vertices contained in the "Vertex Segment" and the number of animation shapes
Whether the texture data is compressed
In the icon file, the Icon header is always located at offset 0. Here's the structure of the Icon header:

| Offset | Length | Description                                                                 |
|--------|--------|-----------------------------------------------------------------------------|
| 0000   | uint32 | magic: `0x010000`                                                           |
| 0004   | uint32 | animation_shapes: Number of animation shapes (see Note 1)                  |
| 0008   | uint32 | tex_type: Texture type (see Note 2)                                         |
| 0012   | uint32 | Unknown, fixed value `0x3F800000`                                           |
| 0016   | uint32 | vertex_count: Number of vertices, always a multiple of 3                   |

### Notes

**Note 1**
The icon model has different sets of vertex data for different actions, called **"shapes."**
Rendering different shapes in a loop creates animation effects.

**Note 2**
The purpose of the **Texture type** field is not yet fully understood.
It is a 4-byte integer. Below is a summary of the observed behavior of each bit (may be incomplete or inaccurate):

| Mask | Description                                                                 |
|------|-----------------------------------------------------------------------------|
| 0001 | Unknown                                                                     |
| 0010 | Unknown                                                                     |
| 0100 | Texture data exists in the icon file. Some games (e.g., *ICO*) have no texture data, resulting in a fully black icon. |
| 1000 | Texture data in the icon file is compressed.                                |

---

### 3.3 Vertex Segment

Polygons in PS2 icons are always composed of **triangles formed by three vertices**.  
Since the vertices are arranged according to a specific pattern, polygons can be constructed by reading the vertex data in sequence. Rendering this data with OpenGL or similar APIs produces a wireframe icon.

The **Vertex Segment** contains data for all vertices in the icon.
Each vertex includes:
- Vertex coordinates
- Normal coordinates
- Texture coordinates
- RGBA color data

Therefore, for an icon with **m vertices** and **n shapes**, the Vertex Segment layout is:

#### Vertex Segment Layout (Per Vertex)

Each vertex in the icon model is composed of the following data blocks, stored sequentially:

| Component | Description |
|----------|-------------|
| **Shape 1 Vertex Coordinates** | Vertex position (X, Y, Z) for animation shape 1 |
| **Shape 2 Vertex Coordinates** | Vertex position (X, Y, Z) for animation shape 2 |
| **...** | Additional vertex positions for other animation shapes |
| **Shape n Vertex Coordinates** | Vertex position (X, Y, Z) for animation shape n |
| **Normal Coordinates** | Normal vector (X, Y, Z) used for lighting calculations |
| **Texture Coordinates** | UV coordinates used for texture mapping |
| **Vertex RGBA** | Per-vertex color and alpha (Red, Green, Blue, Alpha) |

**Notes:**

- All shape-specific vertex positions are stored first, one set per animation shape.
- A single normal vector is shared across all shapes for the same vertex index.
- Texture coordinates (UVs) are shared across shapes and define how the texture is applied.
- RGBA values define per-vertex color and transparency.

This layout enables animation by interpolating vertex positions between shapes while
keeping normals, UVs, and colors constant.


#### Vertex Coordinates

Each vertex coordinate occupies **8 bytes** and has the following structure:

| Offset | Length | Description                                      |
|--------|--------|--------------------------------------------------|
| 0000   | int16  | X-coordinate (divide by 4096 when in use)        |
| 0002   | int16  | Y-coordinate (divide by 4096 when in use)        |
| 0004   | int16  | Z-coordinate (divide by 4096 when in use)        |
| 0006   | uint16 | Unknown                                         |

---

#### Normal Coordinates

Each normal coordinate has the **same structure** as the vertex coordinate data.

---

#### Texture Coordinates

Each texture coordinate occupies **4 bytes** and has the following structure:

| Offset | Length | Description                                      |
|--------|--------|--------------------------------------------------|
| 0000   | int16  | U-coordinate (divide by 4096 when in use)        |
| 0002   | int16  | V-coordinate (divide by 4096 when in use)        |

---

#### Vertex RGBA

Each vertex color occupies **4 bytes** and has the following structure:

| Offset | Length | Description                                      |
|--------|--------|--------------------------------------------------|
| 0000   | uint8  | Red (0-255)                                      |
| 0001   | uint8  | Green (0-255)                                    |
| 0002   | uint8  | Blue (0-255)                                     |
| 0003   | uint8  | Alpha (0-255)                                    |

### 3.4 Animation Segment

The exact meaning of much of the **Animation Segment** is not fully understood.  
However, this is not a major concern, as animation can still be implemented using
**vertex coordinate interpolation**.

The Animation Segment consists of:
- an **Animation Header**
- followed by several **Animation Frames**
- each Animation Frame contains multiple **Key Frames**

#### Animation Segment Structure

The Animation Segment consists of an **Animation Header** followed by multiple
**Animation Frames**. Each animation frame contains **frame metadata** and a list
of **key frames**.

---

#### Overall Layout

|            | Frame Data | Key Frame 1 | Key Frame 2 | ... | Key Frame n |
|------------|------------|-------------|-------------|---|-------------|
| **Frame 1** | Frame Data 1 | Frame Key 1 | Frame Key 2 | ... | Frame Key n |
| **Frame 2** | Frame Data 2 | Frame Key 1 | Frame Key 2 | ... | Frame Key n |
| **...**      | ...          | ...           | ...           | ... | ...           |
| **Frame m** | Frame Data m | Frame Key 1 | Frame Key 2 | ... | Frame Key n |

---

#### Explanation

- Each **Animation Frame** begins with a **Frame Data block**
  (shape ID, key count, and unknown values).
- Following the Frame Data block are **n Key Frames**.
- Each **Key Frame** defines a `(time, value)` pair.
- The total number of animation frames typically matches the number of shapes.
- Animation can be reproduced by interpolating vertex data between shapes
  according to key frame timing.

This structure allows flexible animation playback even though the exact semantic
meaning of some fields remains undocumented.


---

#### Animation Header

| Offset | Length  | Description                                                                 |
|--------|---------|-----------------------------------------------------------------------------|
| 0000   | uint32  | Magic: `0x01`                                                               |
| 0004   | uint32  | Frame Length: Number of frames required to complete one animation cycle     |
| 0008   | float32 | Anim Speed: Play speed (purpose unknown)                                    |
| 0012   | uint32  | Play Offset: Starting frame (purpose unknown)                               |
| 0016   | uint32  | Frame Count: Total number of animation frames; typically one per shape      |

---

#### Frame Data

Frame Data immediately follows the Animation Header.

| Offset | Type | Description            |
|--------|------|------------------------|
| 0000   | u32  | Shape ID               |
| 0004   | u32  | Number of keys         |
| 0008   | u32  | Unknown                |
| 0012   | u32  | Unknown                |

---

#### Key Frame

| Offset | Type | Description |
|--------|------|-------------|
| 0000   | f32  | Time        |
| 0004   | f32  | Value       |

---

### 3.5 Texture Segment

Textures are images with dimensions **128x128 pixels**, encoded using the **TIM**
image format. Based on the `tex_type` field in the Icon Header, textures can be
classified as **uncompressed** or **compressed**.

---

#### Uncompressed Texture

Uncompressed textures use the **BGR555** pixel format:

- Each of **B**, **G**, and **R** occupies **5 bits**
- Total: **15 bits**, stored in **2 bytes** (1 unused bit)

#### Byte Layout

```
High-order byte: Low-order byte:
X B B B B B G G G G G R R R R R
```

- **X** = Don't care  
- **R** = Red  
- **G** = Green  
- **B** = Blue  

The raw image size is always:
128 x 128 x 2 bytes

---

#### Conversion to RGB24

To convert from BGR555 to RGB24:

```
High-order byte: Middle-order byte: Low-order byte:
R R R R R 0 0 0 G G G G G 0 0 0 B B B B B 0 0 0
```


When converting 5-bit color values to 8-bit:
- Pad the lower **3 bits with zeros**

After conversion:
- **RGB24** -> 3 bytes per pixel
- **RGBA32** -> 4 bytes per pixel (with added alpha)

#### Compressed Texture

Compressed textures use a **simple RLE (Run-Length Encoding)** algorithm.

#### Compression Layout

- The first **u32** value specifies the **size of the compressed texture data**
- The remaining data consists of repeating pairs:
  - **u16 rle_code**
  - **u16 rle_data**
- These pairs continue until the compressed data size is exhausted

---

#### RLE Decoding Rules

The `rle_code` determines how `rle_data` is expanded.
Each RLE entry represents **x copies of rle_data repeated y times**.

| Condition                      | x (data count)           | y (repeat count) |
|--------------------------------|--------------------------|------------------|
| `rle_code < 0xFF00`            | 1                        | `rle_code`      |
| `rle_code >= 0xFF00`           | `0x10000 - rle_code`     | 1                |

In other words:
- **Small rle_code** -> repeat a single value many times
- **Large rle_code** -> copy multiple values once

---

#### Decompression Example

- `rle_code = 3`
  - Output: repeat `rle_data` **3 times**

- `rle_code = 65533 (0xFFFD)`
  - `0x10000 - 65533 = 3`
  - Output: copy **3 consecutive data values** once

After decompression, the texture data matches the uncompressed
**128x128x2-byte BGR555** format.

---

#### Post-Decompression Processing

Once decompressed, the texture can be converted using the same method as
uncompressed textures:
- **RGB24** (3 bytes per pixel)
- **RGBA32** (4 bytes per pixel)

See **Section 3.5 Texture Segment** for pixel format conversion details.

---

## 05 References

- **gothi** - `icon.sys` format
- **Martin Åkesson** - *PS2 Icon Format v0.5*
