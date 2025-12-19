# myMCpp

**myMCpp** is a PlayStation 2 memory card manager rewritten from the ground up. It supports `.ps2` images (used by PCSX2) and `.mc2` files (used by the MemCard PRO2).

This is a complete C++ rewrite of the original python-based [mymc++](https://github.com/Adubbz/mymcplusplus), featuring a Qt GUI for better usability.

<!-- TODO: Make screenshot before release
![Screenshot](docs/screenshot.png) -->

## Installation

Since this is a C++ project, you'll generally need to build it from source unless you've downloaded a pre-built release.

Please refer to **[build.md](docs/build.md)** for detailed compilation instructions for Windows, macOS, and Linux (Android/iOS/UWP ports planned).

## Usage

### Graphical Interface
To launch the GUI, just run the executable without any arguments:

```
myMCpp
```

This will open the GUI where you can easily manage, drag, and drop your save files.

### Command Line Interface (CLI)
For those who prefer the terminal or need automation, myMCpp includes a fully-featured CLI.

**Syntax:**
```bash
myMCpp [-ih] memcard.ps2 command [...]
```

**Examples:**
Create a new memory card image:
```bash
myMCpp -i new_card.ps2 format
```

**Available Commands:**
- `gui`: Starts the graphical interface.
- `format`: Creates a new memory card image.
- `dir`: Lists files with details.
- `ls`: simple list of directory contents.
- `export`: Export saves from the card.
- `import`: Import saves to the card.
- `add`: Add files to the card.
- `extract`: Extract specific files.
- `delete` / `remove`: Delete files or directories.
- `mkdir`: Create a directory.
- `df`: Display free space.
- `check`: Check for file system errors.

**Flags:**
- `-h`, `--help`: Show the help message.
- `-i`: Ignore ECC checks (useful for reading some modified cards).
- `-e`: Create without ECC (useful for virtual cards like the SD2PSX).

## License

myMCpp is released under the **GPLv3**. It is **not** Public Domain.

## Contributing

Contributions are welcome! Please feel free to open an issue or submit a pull request.