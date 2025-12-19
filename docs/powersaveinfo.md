## PS2 PowerSave File Format

This file format is used by Action Replay Max / PS2 PowerSave to store a compressed PlayStation 2 save directory. The file consists of a fixed header followed by lzAri-compressed archive data. All strings are null-terminated ASCII unless otherwise stated.

### Header

| Offset | Size | Description |
|------:|-----:|------------|
| 0x00000000 | 12 | Magic string `Ps2PowerSave` |
| 0x0000000C | 4 | CRC32 checksum of entire file (checksum field treated as `0x00000000` during calculation) |
| 0x00000010 | 32 | Save directory name (null-terminated ASCII) |
| 0x00000030 | 32 | `icon.sys` filename (ASCII, null-terminated) |
| 0x00000050 | 4 | Size of compressed data (bytes) |
| 0x00000054 | 4 | Number of files in save archive |
| 0x00000058 | 4 | Size of uncompressed data (bytes) |
| 0x0000005C | Variable | lzAri-compressed archive data |

---

### Archive (after decompression)

After decompressing the data using the lzAri algorithm, the result is an archive containing the actual save files. The archive consists of repeated file entries, one per file.

| Offset | Size | Description |
|------:|-----:|------------|
| 0x00000000 | 4 | File data size (bytes) |
| 0x00000004 | 32 | Filename (ASCII) |
| 0x00000024 | Variable | File data |
| — | Variable | Padding |

---

### Padding

Padding is added after each file entry to maintain alignment. Padding bytes are `0x00`.