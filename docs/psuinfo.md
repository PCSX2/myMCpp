## PS2 PSU Save File Format

<!-- based on data from here: https://www.ps2savetools.com/documents/ps2-save-game-format-for-ems-adapter-psu/ -->

The PSU file format was originally developed for the EMS PS2 Memory Adapter to store uncompressed PlayStation 2 saves. It contains a sequential list of file system entries representing the directory, directory links (`.` and `..`), and files within the save.

All multi-byte integer values in the header are stored in big-endian format.

---

## Overall File Layout

A valid PSU file consists of a main directory entry, followed by `.` and `..` directory entries, and finally the individual file entries:

1. **Main Directory Entry**: Defines the name of the save directory and the total count of entries.
2. **Current Directory Entry (`.`)**: Required link to the current directory.
3. **Parent Directory Entry (`..`)**: Required link to the parent directory.
4. **File Entries**: Individual files belonging to the save directory.

The total number of entries in the file matches the count specified in the Main Directory Entry.

---

## File System Entry Structure

Each entry in a PSU file represents either a directory or a file. It has a fixed-size header area of 512 bytes, followed by the variable-length content and alignment padding.

| Offset | Size (bytes) | Description |
|-------:|-------------:|-------------|
| 0x0000 | 32 | File entry header |
| 0x0020 | 32 | Reserved (padded with `0x00`) |
| 0x0040 | 448 | Entry name (ASCII, right-filled with `0x00`) |
| 0x0200 | Variable | File content (0 bytes for directory, `.`, and `..` entries) |
| Variable | Variable | Filler bytes (pads the file content to a multiple of 1024 bytes) |

### Alignment Padding (Filler Bytes)

To maintain 1024-byte alignment, filler bytes are appended after the file content block:
- For directory entries, since the content size is 0, no filler bytes are required.
- For files, the number of required filler bytes is calculated as:
  `Filler Bytes = (1024 - (File Size % 1024)) % 1024`
- Filler bytes are usually `0x00`, `0xCD`, or `0xFF`, but their exact value is ignored by parsers.

---

## 32-Byte Header Layout

| Offset | Size (bytes) | Description |
|-------:|-------------:|-------------|
| 0x00 | 2 | Entry Type ID (see [Entry Type ID](#entry-type-id)) |
| 0x02 | 1 | Unknown |
| 0x03 | 2 | Unknown (usually `0x0000`) |
| 0x05 | 4 | File Size or Entry Count (big-endian) |
| 0x09 | 7 | Creation Date (see [Date and Time Format](#date-and-time-format)) |
| 0x10 | 2 | Start sector address on memory card (big-endian) |
| 0x12 | 7 | Unknown (possibly metadata or checksum) |
| 0x19 | 7 | Last Modified Date (same format as Creation Date) |

### Entry Type ID

The first two bytes of the header define the type of the entry:

- **`0x2784`**: Directory Entry (used for the main save directory, as well as `.` and `..` entries).
- **`0x9784`**: Standard File Entry.
- **`0x27A0`**: PS2 System Save File (e.g., `BEDATA-SYSTEM`).

### File Size or Entry Count

For directory entries (Type `0x2784`), this field specifies the total number of files/entries contained in the directory (including the `.` and `..` directory links). For standard files, it specifies the file size in bytes.

### Date and Time Format

Dates are represented as a 7-byte structure:

| Offset | Description | Range |
|-------:|-------------|-------|
| 0x00 | Seconds | 0-59 |
| 0x01 | Minutes | 0-59 |
| 0x02 | Hours | 0-23 |
| 0x03 | Day | 1-31 |
| 0x04 | Month | 1-12 |
| 0x05 | Year (Low Byte) | 0-255 |
| 0x06 | Year (High Byte) | 0-255 |

The year is represented as a 16-bit integer split across two bytes (e.g., `Year = (High Byte << 8) | Low Byte`).
