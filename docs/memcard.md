# PlayStation 2 Memory Card File System

<!-- http://www.csclub.uwaterloo.ca:11068/mymc/ps2mcfs.html -->

**By Ross Ridge**  
**Public Domain**

This document describes the file system layout used on PlayStation 2 memory cards. It is based on research conducted while writing **mymc**, a utility for working with PS2 memory card images. This document attempts to be comprehensive and accurate, but some details may be missing, misleading, or incorrect. Many assumptions were required during the research process, and it is difficult to know Sony's exact intent in all cases. Almost all structure, field, and flag names were coined by the author. Nothing in this document should be considered official.

For brevity, unused fields and flag bits are omitted from tables. In most cases, unused fields or flags should be assumed to be reserved or padding and set to zero when writing. Structures must be padded to the length specified at the top of each table. All values are stored on the card using **little-endian** byte order.

---

## NAND Flash Memory Basics

### Glossary

- **block**
  See *erase block*.

- **cluster**
  The unit of allocation used in the file system. A cluster is one or more pages in size.

- **ECC**
  Error Correcting Code. A method of encoding data so random bit errors can be detected and corrected.

- **erase block**
  The basic erasable unit on a memory card.

- **half**
  A two-byte unsigned half-word value.

- **page**
  The basic addressable unit on a memory card. Corresponds to a page on the flash device and is analogous to a sector on a hard disk.

- **programming**
  The operation of changing erased bits on a flash device from `1` to `0`.

- **superblock**
  The first page on the memory card containing critical file system structure information.

- **word**
  A four-byte unsigned word value.

---

### NAND Flash Characteristics

PlayStation 2 memory cards use **NAND flash**, a non-volatile memory type that can be electrically erased and reprogrammed. NAND flash is ideal for memory cards but has several limitations that influence file system design.

#### Access Speed

Random access is relatively slow. Reading the first byte takes ~25 µs, while sequential reads are much faster (~50 ns per byte). For example, the **TC58V64AFT** flash device (used in PS2 memory cards) can read a full 528-byte page at ~10 Mb/s sequentially, but only ~40 KB/s for random byte reads. Actual transfer rates are lower due to the PS2's serial bus limitations.

#### Write Limitations

Flash memory can only change bits from `1` to `0`. To reset bits back to `1`, an entire **erase block** must be erased. Since erase blocks contain multiple pages, writing a single page requires:

1. Reading all pages in the erase block
2. Erasing the block
3. Reprogramming all pages with updated data

Some flash devices reverse this behavior (erasing to `0`, programming to `1`).

#### Reliability

Flash memory is less reliable than RAM:

- Devices may ship with bad blocks
- New defects may develop over time
- Blocks wear out after many erase/program cycles

Each page is split into:

- **512 bytes data**
- **16 bytes spare area** (ECC, wear leveling, bad block tracking)

---

### Flash Geometry (PS2 Memory Cards)

- Page size: **528 bytes**
  - Data area: 512 bytes
  - Spare area: 16 bytes
- Erase block: **16 pages**
- Total pages: **16,384**
- Total raw data capacity: **8,388,608 bytes**

---

## File System Organization

### Standard Memory Card Layout

0x0000 Superblock
0x0001 Unused

0x0010 Indirect FAT Table
0x0012 FAT Table

0x0052 Allocatable Clusters

0x3ED2 Reserved Clusters

0x3FE0 Backup Block 2
0x3FF0 Backup Block 1
0x3FFF


The PS2 memory card file system is similar to the MS-DOS FAT file system:

- Uses a **File Allocation Table (FAT)**
- Hierarchical directory structure
- Clusters group multiple flash pages

On standard PS2 memory cards:

- Cluster size: **1024 bytes**
- Pages per cluster: **2**

---

## The Superblock

The superblock is the only structure with a fixed location. It resides in the **first page** of the memory card.

### Superblock Layout (340 bytes)

| Offset | Name | Type | Default | Description |
|------:|------|------|---------|-------------|
| 0x00 | magic | byte[28] | - | ASCII string `"Sony PS2 Memory Card Format "` |
| 0x1C | version | byte[12] | 1.X.0.0 | Format version |
| 0x28 | page_len | half | 512 | Page size in bytes |
| 0x2A | pages_per_cluster | half | 2 | Pages per cluster |
| 0x2C | pages_per_block | half | 16 | Pages per erase block |
| 0x2E | - | half | 0xFF00 | Unused |
| 0x30 | clusters_per_card | word | 8192 | Total clusters |
| 0x34 | alloc_offset | word | 41 | First allocatable cluster |
| 0x38 | alloc_end | word | 8135 | End of allocatable clusters |
| 0x3C | rootdir_cluster | word | 0 | Root directory cluster |
| 0x40 | backup_block1 | word | 1023 | Backup erase block |
| 0x44 | backup_block2 | word | 1022 | Backup erase block |
| 0x50 | ifc_list | word[32] | 8 | Indirect FAT cluster list |
| 0xD0 | bad_block_list | word[32] | -1 | Bad erase blocks |
| 0x150 | card_type | byte | 2 | Must be PS2 |
| 0x151 | card_flags | byte | 0x52 | Card characteristics |

### Card Flags

| Mask | Name | Description |
|-----:|------|-------------|
| 0x01 | CF_USE_ECC | ECC supported |
| 0x08 | CF_BAD_BLOCK | Bad blocks possible |
| 0x10 | CF_ERASE_ZEROES | Erased blocks are zeroed |

---

## File Allocation Table (FAT)

Each FAT entry is a 32-bit value:

- MSB clear -> cluster free
- MSB set -> cluster allocated
- Lower 31 bits -> next cluster index (relative to `alloc_offset`)
- `0xFFFFFFFF` -> end of file

### FAT Indirection

The FAT uses **double-indirect indexing**:

- `ifc_list` -> indirect FAT clusters
- Indirect clusters -> FAT clusters
- FAT clusters -> FAT entries

Example access logic (cluster size = 1024):

```c
fat_offset = fat_index % 256;
indirect_index = fat_index / 256;
indirect_offset = indirect_index % 256;
dbl_indirect_index = indirect_index / 256;

indirect_cluster_num = superblock.ifc_table[dbl_indirect_index];
indirect_cluster = read_cluster(indirect_cluster_num);

fat_cluster_num = indirect_cluster[indirect_offset];
fat_cluster = read_cluster(fat_cluster_num);

entry = fat_cluster[fat_offset];

---

## Directories

Directories are files containing directory entries. The root directory cluster is specified by rootdir_cluster.

### Directory Entry Mode Flags

| Mask | Name | Description |
|-----:|------|-------------|
| 0x0001 | DF_READ | Read permission |
| 0x0002 | DF_WRITE | Write permission |
| 0x0004 | DF_EXECUTE | Execute permission |
| 0x0008 | DF_PROTECTED | Copy protected |
| 0x0010 | DF_FILE | Regular file |
| 0x0020 | DF_DIRECTORY | Directory |
| 0x0800 | DF_POCKETSTN | PocketStation save |
| 0x1000 | DF_PSX | PS1 save |
| 0x2000 | DF_HIDDEN | Hidden file |
| 0x8000 | DF_EXISTS | Entry in use |

### Directory Entry Layout (512 bytes)

| Offset | Name | Type | Description |
|------:|------|------|-------------|
| 0x00 | mode | half | Mode flags |
| 0x04 | length | word | File size or entry count |
| 0x08 | created | byte[8] | Creation time |
| 0x10 | cluster | word | First cluster |
| 0x14 | dir_entry | word | Parent directory entry |
| 0x18 | modified | byte[8] | Modification time |
| 0x20 | attr | word | User attribute |
| 0x40 | name | byte[32] | Null-terminated name |

### Time of Day Format (8 bytes)

| Offset | Name | Type | Description |
|------:|------|------|-------------|
| 0x01 | sec | byte | Seconds |
| 0x02 | min | byte | Minutes |
| 0x03 | hour | byte | Hours |
| 0x04 | day | byte | Day |
| 0x05 | month | byte | Month (1-12) |
| 0x06 | year | word | Year (4-digit) |

All timestamps use Japan Standard Time (UTC+9).

### Error Management

#### Error Correction Code (ECC)

Data divided into four 128-byte chunks

Each chunk uses a 20-bit Hamming code

ECC stored in spare area (12 bytes total)

#### Backup Blocks

Two erase blocks ensure atomic writes:

Erase backup blocks

Backup new data to backup_block1

Write target block number to backup_block2

Program target block

Erase backup_block2

Recovery occurs on card insertion if needed.

#### Bad Block List

Stored in bad_block_list in the superblock. Blocks listed are avoided for new allocations.

#### Reserved Clusters

The PS2 rounds usable clusters down to the nearest thousand, creating hidden replacement clusters. As blocks fail, reserved clusters become available, maintaining consistent reported capacity.

### See Also

#### NAND Flash Memory

Micron: NAND Flash 101

Wikipedia: Flash memory

#### Error Correction Codes

STMicroelectronics: ECC in SLC NAND

Micron: Hamming Codes for NAND Flash

Hanimar: Sample ECC C code