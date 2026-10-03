## iconsysinfo

<!-- based on data from here: https://www.ps2savetools.com/documents/iconsys-format/ -->

This structure defines how a PlayStation 2 save appears in the PS2 Browser, including background gradients, lighting, title text, and icon filenames. All offsets are relative to the start of the file.

### General Notes:
- All multi-byte values are little-endian
- Background RGB channels use 0x00-0xFF (0-255), transparency uses 0x00-0x80 (0-128)
- Background opacity comes from the transparency field at offset 12
- Lighting directions, light colors, and ambient light are four 32-bit floats each
- Strings are null-terminated
- Text encoding for titles is Shift-JIS
- Total file size is 964 bytes

| Offset (bytes) | Size (bytes) | Description                                                     |
| -------------: | -----------: | --------------------------------------------------------------- |
|              0 |            4 | Magic (`PS2D`)                                                  |
|              4 |            2 | Unknown                                                         |
|              6 |            2 | Offset of 2nd line in title name                                |
|              8 |            4 | Unknown                                                         |
|             12 |            4 | Background transparency (`0x00` = transparent, `0x80` = opaque) |
|             16 |           16 | Background color - upper left (four uint32 values, RGB-)        |
|             32 |           16 | Background color - upper right (four uint32 values, RGB-)       |
|             48 |           16 | Background color - lower left (four uint32 values, RGB-)        |
|             64 |           16 | Background color - lower right (four uint32 values, RGB-)       |
|             80 |           16 | Light 1 direction (float32[4])                                  |
|             96 |           16 | Light 2 direction (float32[4])                                  |
|            112 |           16 | Light 3 direction (float32[4])                                  |
|            128 |           16 | Light 1 color (float32[4])                                      |
|            144 |           16 | Light 2 color (float32[4])                                      |
|            160 |           16 | Light 3 color (float32[4])                                      |
|            176 |           16 | Ambient light color (float32[4])                                |
|            192 |           68 | Save title name (null-terminated, Shift-JIS)                    |
|            260 |           64 | Normal icon filename (null-terminated)                          |
|            324 |           64 | Copy icon filename (null-terminated)                            |
|            388 |           64 | Delete icon filename (null-terminated)                          |
|            452 |          512 | Reserved / padding                                              |
