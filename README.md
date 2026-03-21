<p align="center">
	<img src="resources/icons/AppIcon.png" alt="myMCpp logo" width="120" />
</p>

<h1 align="center">myMCpp</h1>

<p align="center">
	<a href="https://crowdin.com/project/mymcpp">
		<img src="https://badges.crowdin.net/mymcpp/localized.svg" alt="Crowdin" />
	</a>
</p>

<p align="center">
	<code>myMCpp</code> is a PlayStation 2 memory card manager. It works with <code>.ps2</code> images (PCSX2) and <code>.mc2</code> files (MemCard PRO2).
</p>

<p align="center">
	This project is in <strong>alpha</strong>. It is <strong>experimental</strong>, <strong>unstable</strong>, and <strong>not intended for production or general use</strong>. Interfaces and behavior may change at any time.
</p>

<p align="center">
	The project is a full C++ rewrite of the original Python based mymc++, with a Qt GUI and CLI.
</p>

## Usage

### GUI 

Just run `myMCpp` without arguments or double click it to launch the GUI. You can open memory card images, view and manage saves, and export/import files.

### CLI

```zsh
myMCpp [options] memcard.ps2 command [...]
```

Example:

```zsh
myMCpp -i new_card.ps2 format
```

Commands:
- `format`: Create a new memory card image.
- `dir`: List files with details.
- `ls`: List directory contents.
- `export`: Export saves from the card.
- `import`: Import saves to the card.
- `add`: Add files to the card.
- `extract`: Extract specific files.
- `delete` / `remove`: Delete files or directories.
- `mkdir`: Create a directory.
- `df`: Show free space.
- `check`: Check for file system errors.
- `ecc_check`: Check ECC status and validate with a file system check.
- `clear`: Clear mode flags on files/directories.
- `set`: Set mode flags on files/directories.

Global options:
- `--version`: Show version information.
- `-h`, `--help`: Show help.
- `-i`, `--ignore-ecc`: Ignore ECC errors while reading.
- `-e`, `--no-ecc`: Create without ECC (useful for virtual cards such as SD2PSX).

## License

Licensed under GPLv3. See [LICENSE](LICENSE).

## Credits and Origins

- `myMCpp` is a C++ rewrite inspired by **mymc++**, which itself builds on **mymc+** by Florian Märkl and the original **mymc** utility created by Ross Ridge (public domain).

- Documentation about PS2 icons and save formats in the `docs/` directory is based on community research, including material from **PS2 Save Tools** (https://www.ps2savetools.com) and other references linked in the individual files.

- The application icons in `resources/icons` come from **Remix Icon** and are used under the **Apache 2.0** license. See `resources/icons/white/svg/LICENSE` and `resources/licenses.html` for the full license text and attribution details.

## Contributing

Issues and pull requests are welcome.
You can also help with translations on [Crowdin](https://crowdin.com/project/mymcpp).