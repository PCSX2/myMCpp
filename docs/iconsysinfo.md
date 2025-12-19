This structure defines how a PlayStation 2 save appears in the PS2 Browser, including background gradients, lighting, title text, and icon filenames. All offsets are relative to the start of the file.

### General Notes:
- All multi-byte values are big-endian
- Color values use RGB- format with a range of 0x00–0x80
- Strings are null-terminated
- Text encoding for titles is Shift-JIS
- Total file size is 964 bytes

| Offset (bytes) | Size (bytes) | Description |
|---------------:|-------------:|------------|
| 0              | 4            | Magic (`PS2D`) |
| 4              | 2            | Unknown |
| 6              | 2            | Offset of 2nd line in title name |
| 8              | 4            | Unknown |
| 12             | 4            | Background transparency when **Save** is selected in PS2 Browser (`0x00` = transparent, `0x80` = opaque) |
| 16             | 16           | Background color – upper left (RGB-, `0x00`–`0x80`) |
| 32             | 16           | Background color – upper right (RGB-, `0x00`–`0x80`) |
| 48             | 16           | Background color – lower left (RGB-, `0x00`–`0x80`) |
| 64             | 16           | Background color – lower right (RGB-, `0x00`–`0x80`) |
| 80             | 16           | Light 1 direction |
| 96             | 16           | Light 2 direction |
| 112            | 16           | Light 3 direction |
| 128            | 16           | Light 1 color (RGB-) |
| 144            | 16           | Light 2 color (RGB-) |
| 160            | 16           | Light 3 color (RGB-) |
| 176            | 16           | Ambient light color (RGB-) |
| 192            | 68           | Save title name (null-terminated, Shift-JIS) |
| 260            | 64           | Normal icon filename (null-terminated) |
| 324            | 64           | Copy icon filename (null-terminated) |
| 388            | 64           | Delete icon filename (null-terminated) |
| 452            | 512          | Reserved / padding |
