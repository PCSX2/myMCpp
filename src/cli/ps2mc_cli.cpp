// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "ps2mc_cli.h"
#include "ps2mc.h"
#include "ps2save.h"
#include "../common/Logger.h"
#include "version.h"
#include "BuildVersion.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

PS2McCommandLine::PS2McCommandLine() = default;
PS2McCommandLine::~PS2McCommandLine() = default;

void PS2McCommandLine::printHelp()
{
	std::cout << "Usage: myMCpp [-i memcard] [-h] command [arguments]\n\n"
			  << "Manipulate PS2 memory card images.\n\n"
			  << "Supported commands:\n"
			  << "   add:     Add files to the memory card\n"
			  << "   check:   Check for file system errors\n"
			  << "   clear:   Clear mode flags on files and directories\n"
			  << "   delete:  Recursively delete a directory (save file)\n"
			  << "   df:      Display free space\n"
			  << "   dir:     Display save file information\n"
			  << "   ecc_check: Scan for ECC errors on all pages\n"
			  << "   export:  Export save files from the memory card\n"
			  << "   extract: Extract files from the memory card\n"
			  << "   format:  Create a new memory card image\n"
			  << "   import:  Import save files into the memory card\n"
			  << "   ls:      List the contents of a directory\n"
			  << "   mkdir:   Make directories\n"
			  << "   remove:  Remove files and directories\n"
			  << "   set:     Set mode flags on files and directories\n"
			  << "\nOptions:\n"
			  << "   --version         Show version information\n"
			  << "   -h, --help        Show this help message\n"
			  << "   -i, --ignore-ecc  Ignore ECC errors while reading\n"
			  << "   -e, --no-ecc      Create virtual memory card without ECC\n";
}

void PS2McCommandLine::printVersion()
{
	std::string rev = BuildVersion::GitRev ? BuildVersion::GitRev : "";
	std::string hash = BuildVersion::GitHash ? BuildVersion::GitHash : "";

	std::cout << "myMCpp " << myMCpp_VERSION_STRING;

	if (!rev.empty() && rev != "Unknown")
	{
		std::cout << " (" << rev << ")";
	}
	else if (!hash.empty())
	{
		if (hash.size() > 7)
			hash = hash.substr(0, 7);
		std::cout << " (git " << hash << ")";
	}

	std::cout << "\n";
}

int PS2McCommandLine::execute(int argc, char* argv[])
{
	if (argc < 2)
	{
		printHelp();
		return 1;
	}

	std::vector<std::string> args(argv + 1, argv + argc);
	size_t i = 0;
	std::string memcardPath;

	while (i < args.size())
	{
		const auto& arg = args[i];

		if (arg == "-h" || arg == "--help")
		{
			printHelp();
			return 0;
		}
		else if (arg == "--version")
		{
			printVersion();
			return 0;
		}
		else if (arg == "-i" || arg == "--ignore-ecc")
		{
			ignoreEcc = true;
			++i;
		}
		else if (arg == "-e" || arg == "--no-ecc")
		{
			noEcc = true;
			++i;
		}
		else if (arg.substr(0, 1) != "-" || arg == "-")
		{
			// First non-option argument should be the memcard path
			break;
		}
		else
		{
			Logger::error("Unknown option: {}", arg);
			return 1;
		}
	}

	if (i < args.size() && args[i].substr(0, 1) != "-")
	{
		memcardPath = args[i];
		currentMemCardPath = memcardPath;
		++i;
	}

	if (i >= args.size())
	{
		Logger::error("No command specified");
		printHelp();
		return 1;
	}

	std::string command = args[i];
	++i;

	std::vector<std::string> cmdArgs(args.begin() + i, args.end());

	if (!memcardPath.empty() && command != "format")
	{
		try
		{
			memoryCard = std::make_unique<PS2MemoryCard>();
			memoryCard->open(memcardPath);
		}
		catch (const std::exception& e)
		{
			Logger::error("Error opening memory card: {}", e.what());
			return 1;
		}
	}

	try
	{
		if (command == "ls")
			return cmdLs(cmdArgs);
		else if (command == "dir")
			return cmdDir(cmdArgs);
		else if (command == "extract")
			return cmdExtract(cmdArgs);
		else if (command == "add")
			return cmdAdd(cmdArgs);
		else if (command == "import")
			return cmdImport(cmdArgs);
		else if (command == "export")
			return cmdExport(cmdArgs);
		else if (command == "delete")
			return cmdDelete(cmdArgs);
		else if (command == "remove")
			return cmdRemove(cmdArgs);
		else if (command == "mkdir")
			return cmdMkdir(cmdArgs);
		else if (command == "format")
		{
			if (memcardPath.empty())
			{
				Logger::error("Memory card path required for format");
				return 1;
			}
			return cmdFormat({memcardPath});
		}
		else if (command == "check")
			return cmdCheck(cmdArgs);
		else if (command == "df")
			return cmdDf(cmdArgs);
		else if (command == "clear")
			return cmdClear(cmdArgs);
		else if (command == "set")
			return cmdSet(cmdArgs);
		else if (command == "ecc_check")
			return cmdEccCheck(cmdArgs);
		else if (command == "gui")
		{
			Logger::error("GUI mode not available in CLI build");
			return 1;
		}
		else
		{
			Logger::error("Unknown command: {}", command);
			printHelp();
			return 1;
		}
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdLs(const std::vector<std::string>& args)
{
	if (!memoryCard)
	{
		Logger::error("No memory card open");
		return 1;
	}

	std::string path = args.empty() ? "/" : args[0];
	try
	{
		auto entries = memoryCard->listDir(path);

		const char* modeChars = "rwxpfdD81C+KPH4";

		for (const auto& ent : entries)
		{
			if (!(ent.mode & 0x8000))
				continue; // DF_EXISTS

			// Print mode
			for (int b = 0; b < 15; ++b)
			{
				std::cout << ((ent.mode & (1 << b)) ? modeChars[b] : '-');
			}
			std::cout << " ";

			std::cout << std::setw(7) << ent.length << " ";

			std::cout << std::setfill('0')
					  << std::setw(4) << (1900 + ent.modified.year) << "-"
					  << std::setw(2) << (int)ent.modified.month << "-"
					  << std::setw(2) << (int)ent.modified.mday << " "
					  << std::setw(2) << (int)ent.modified.hour << ":"
					  << std::setw(2) << (int)ent.modified.min << ":"
					  << std::setw(2) << (int)ent.modified.sec << " "
					  << std::setfill(' ') << ent.name << "\n";
		}
		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdDir(const std::vector<std::string>& args)
{
	(void)args; // args not used for dir command
	if (!memoryCard)
	{
		Logger::error("No memory card open");
		return 1;
	}

	try
	{
		auto entries = memoryCard->listDir("/");

		for (const auto& ent : entries)
		{
			if (!(ent.mode & DF_DIR) || (ent.mode & DF_HIDDEN))
				continue;
			if (ent.name == "." || ent.name == "..")
				continue;

			std::string dirPath = "/" + ent.name;

			std::string title, subtitle;
			try
			{
				if (ent.mode & DF_PSX)
				{
					title = memoryCard->getPsxTitle(dirPath);
					if (title.empty())
						title = "Corrupt";
				}
				else
				{
					title = memoryCard->getSaveTitle(dirPath);
					if (title.empty())
						title = "Corrupt";
					subtitle = memoryCard->getSaveSubtitle(dirPath);
				}
			}
			catch (...)
			{
				title = "Corrupt";
			}

			uint32_t sizeBytes = memoryCard->getSaveSize(dirPath);
			uint32_t sizeKB = sizeBytes / 1024;

			std::string protection;
			uint16_t protBits = ent.mode & (DF_PROTECTED | DF_WRITE);
			if (protBits == 0)
				protection = "Delete Protected";
			else if (protBits == DF_WRITE)
				protection = "Not Protected";
			else if (protBits == DF_PROTECTED)
				protection = "Copy & Delete Protected";
			else
				protection = "Copy Protected";

			// Check for PSX/PocketStation type
			if (ent.mode & DF_PSX)
			{
				if (ent.mode & DF_POCKETSTN)
					protection = "PocketStation";
				else
					protection = "PlayStation";
			}

			// Print first line: dirname  title
			std::cout << std::left << std::setw(32) << ent.name << " " << title << "\n";

			// Print second line: size  protection  subtitle
			std::cout << std::right << std::setw(4) << sizeKB << "KB "
					  << std::left << std::setw(25) << protection << " " << subtitle << "\n";

			std::cout << "\n";
		}

		// Print free space summary
		uint32_t freeSpace = memoryCard->getFreeSpace() / 1024;
		std::string freeStr;
		if (freeSpace > 999999)
		{
			freeStr = std::to_string(freeSpace / 1000000) + "," +
			          std::to_string((freeSpace / 1000) % 1000) + "," +
			          std::to_string(freeSpace % 1000);
		}
		else if (freeSpace > 999)
		{
			freeStr = std::to_string(freeSpace / 1000) + "," +
			          std::to_string(freeSpace % 1000);
		}
		else
		{
			freeStr = std::to_string(freeSpace);
		}
		std::cout << freeStr << " KB Free\n";

		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdExtract(const std::vector<std::string>& args)
{
	if (!memoryCard || args.empty())
	{
		Logger::error("No memory card open or filename required");
		return 1;
	}

	try
	{
		auto data = memoryCard->readFile(args[0]);
		std::string outFile = args.size() > 1 ? args[1] : fs::path(args[0]).filename().string();

		std::ofstream out(outFile, std::ios::binary);
		out.write(reinterpret_cast<const char*>(data.data()), data.size());
		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdAdd(const std::vector<std::string>& args)
{
	if (!memoryCard || args.empty())
	{
		Logger::error("No memory card open or filename required");
		return 1;
	}

	try
	{
		for (const auto& filepath : args)
		{
			std::ifstream in(filepath, std::ios::binary);
			if (!in)
			{
				Logger::error("Cannot open file: {}", filepath);
				return 1;
			}

			std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
				std::istreambuf_iterator<char>());

			std::string fname = fs::path(filepath).filename().string();
			memoryCard->writeFile("/" + fname, data);
		}
		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdImport(const std::vector<std::string>& args)
{
	if (!memoryCard || args.empty())
	{
		Logger::error("No memory card open or save file required");
		return 1;
	}

	try
	{
		int imported = 0;
		int skipped = 0;

		for (const auto& filepath : args)
		{
			PS2SaveFile save;
			save.load(filepath);

			std::string saveName = save.getTitle();
			if (saveName.empty())
			{
				// Get directory name from first entry
				const auto& entries = save.getEntries();
				if (!entries.empty())
				{
					saveName = entries[0].dirEntry.name;
				}
				if (saveName.empty())
				{
					saveName = fs::path(filepath).stem().string();
				}
			}

			std::cout << "Importing " << filepath << " to /" << saveName << "...";

			bool result = memoryCard->importSaveFile(save, false, "");

			if (result)
			{
				std::cout << " OK\n";
				imported++;
			}
			else
			{
				std::cout << " SKIPPED (already exists)\n";
				skipped++;
			}
		}

		if (imported > 0)
		{
			std::cout << "\nImported " << imported << " save(s)";
			if (skipped > 0)
			{
				std::cout << ", skipped " << skipped;
			}
			std::cout << ".\n";
		}

		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdExport(const std::vector<std::string>& args)
{
	if (!memoryCard || args.empty())
	{
		Logger::error("No memory card open or save directory required");
		return 1;
	}

	try
	{
		int exported = 0;

		for (const auto& savePath : args)
		{
			// Ensure path starts with /
			std::string path = savePath;
			if (path.empty() || path[0] != '/')
			{
				path = "/" + path;
			}

			PS2SaveFile save;
			memoryCard->exportSaveFile(path, save);

			std::string basename = path;
			if (basename[0] == '/')
			{
				basename = basename.substr(1);
			}

			std::string outFilename = basename + ".psu";

			std::cout << "Exporting " << path << " to " << outFilename << "...";

			save.save(outFilename, SaveFormat::EMS);

			std::cout << " OK\n";
			exported++;
		}

		if (exported > 0)
		{
			std::cout << "\nExported " << exported << " save(s).\n";
		}

		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdDelete(const std::vector<std::string>& args)
{
	if (!memoryCard || args.empty())
	{
		Logger::error("No memory card open or save name required");
		return 1;
	}

	try
	{
		memoryCard->remove(args[0]);
		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdRemove(const std::vector<std::string>& args)
{
	return cmdDelete(args);
}

int PS2McCommandLine::cmdMkdir(const std::vector<std::string>& args)
{
	if (!memoryCard || args.empty())
	{
		Logger::error("No memory card open or directory path required");
		return 1;
	}

	try
	{
		for (const auto& path : args)
		{
			memoryCard->makeDir(path);
		}
		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdFormat(const std::vector<std::string>& args)
{
	if (args.empty())
	{
		Logger::error("Memory card path required");
		return 1;
	}

	try
	{
		auto mc = std::make_unique<PS2MemoryCard>();
		mc->create(args[0], 8); // 8 MB default
		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdCheck(const std::vector<std::string>& args)
{
	(void)args;
	if (!memoryCard)
	{
		Logger::error("No memory card open");
		return 1;
	}

	try
	{
		bool result = memoryCard->check();
		if (result)
		{
			std::cout << "No errors found.\n";
			return 0;
		}
		else
		{
			std::cout << "Errors found in file system.\n";
			return 1;
		}
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdDf(const std::vector<std::string>& args)
{
	(void)args;
	if (!memoryCard)
	{
		Logger::error("No memory card open");
		return 1;
	}

	try
	{
		uint32_t free = memoryCard->getFreeSpace();
		std::cout << "Free space: " << free << " bytes\n";
		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdClear(const std::vector<std::string>& args)
{
	if (!memoryCard || args.empty())
	{
		Logger::error("Usage: clear [--read] [--write] [--execute] [--protected] [--psx] [--pocketstation] [--hidden] <path>\n\n");
		return 1;
	}

	try
	{
		// Parse flags to clear // Restoring context
		uint16_t clear_mask = 0;
		std::vector<std::string> paths;

		for (const auto& arg : args)
		{
			if (arg == "--read")
				clear_mask |= DF_READ;
			else if (arg == "--write")
				clear_mask |= DF_WRITE;
			else if (arg == "--execute")
				clear_mask |= DF_EXECUTE;
			else if (arg == "--protected")
				clear_mask |= DF_PROTECTED;
			else if (arg == "--psx")
				clear_mask |= DF_PSX;
			else if (arg == "--pocketstation")
				clear_mask |= DF_POCKETSTN;
			else if (arg == "--hidden")
				clear_mask |= DF_HIDDEN;
			else if (!arg.empty() && arg[0] != '-')
				paths.push_back(arg);
		}

		if (clear_mask == 0)
		{
			Logger::error("Error: At least one flag must be specified to clear");
			return 1;
		}

		if (paths.empty())
		{
			Logger::error("Error: At least one path must be specified");
			return 1;
		}

		// Clear flags for each path
		for (const auto& path : paths)
		{
			uint16_t current_mode = memoryCard->getMode(path);
			uint16_t new_mode = current_mode & ~clear_mask;
			memoryCard->setMode(path, new_mode);
			std::cout << "Cleared flags for: " << path << " (0x" << std::hex << current_mode << " -> 0x" << new_mode << std::dec << ")\n";
		}

		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdSet(const std::vector<std::string>& args)
{
	if (!memoryCard || args.empty())
	{
		Logger::error("Usage: set [--read] [--write] [--execute] [--protected] [--psx] [--pocketstation] [--hidden] [-x <hex_value>] <path>...\n");
		return 1;
	}

	try
	{
		// Parse flags to set
		uint16_t set_mask = 0;
		uint16_t clear_mask = 0xFFFF;
		std::vector<std::string> paths;
		std::string hex_value;
		bool use_hex = false;

		for (size_t i = 0; i < args.size(); ++i)
		{
			const auto& arg = args[i];

			if (arg == "--read")
			{
				set_mask |= DF_READ;
				clear_mask &= ~DF_READ;
			}
			else if (arg == "--write")
			{
				set_mask |= DF_WRITE;
				clear_mask &= ~DF_WRITE;
			}
			else if (arg == "--execute")
			{
				set_mask |= DF_EXECUTE;
				clear_mask &= ~DF_EXECUTE;
			}
			else if (arg == "--protected")
			{
				set_mask |= DF_PROTECTED;
				clear_mask &= ~DF_PROTECTED;
			}
			else if (arg == "--psx")
			{
				set_mask |= DF_PSX;
				clear_mask &= ~DF_PSX;
			}
			else if (arg == "--pocketstation")
			{
				set_mask |= DF_POCKETSTN;
				clear_mask &= ~DF_POCKETSTN;
			}
			else if (arg == "--hidden")
			{
				set_mask |= DF_HIDDEN;
				clear_mask &= ~DF_HIDDEN;
			}
			else if (arg == "-x" || arg == "--hex")
			{
				if (i + 1 < args.size())
				{
					hex_value = args[++i];
					use_hex = true;
				}
				else
				{
					Logger::error("Error: -x requires a value");
					return 1;
				}
			}
			else if (!arg.empty() && arg[0] != '-')
			{
				paths.push_back(arg);
			}
		}

		if (paths.empty())
		{
			Logger::error("Error: At least one path must be specified");
			return 1;
		}

		// Determine mode value
		uint16_t mode_value = 0;
		bool use_direct_value = false;

		if (use_hex)
		{
			if (set_mask != 0 || clear_mask != 0xFFFF)
			{
				Logger::error("Error: -x cannot be combined with flag options");
				return 1;
			}

			// Parse hex value
			try
			{
				if (hex_value.substr(0, 2) == "0x" || hex_value.substr(0, 2) == "0X")
				{
					mode_value = static_cast<uint16_t>(std::stoul(hex_value.substr(2), nullptr, 16));
				}
				else
				{
					mode_value = static_cast<uint16_t>(std::stoul(hex_value, nullptr, 16));
				}
				use_direct_value = true;
			}
			catch (...)
			{
				Logger::error("Error: Invalid hex value: {}", hex_value);
				return 1;
			}
		}
		else if (set_mask == 0 && clear_mask == 0xFFFF)
		{
			Logger::error("Error: At least one flag must be specified (or use -x for hex value)");
			return 1;
		}

		// Set mode for each path
		for (const auto& path : paths)
		{
			uint16_t current_mode = memoryCard->getMode(path);
			uint16_t new_mode;

			if (use_direct_value)
			{
				new_mode = mode_value;
			}
			else
			{
				new_mode = (current_mode & clear_mask) | set_mask;
			}

			memoryCard->setMode(path, new_mode);
			std::cout << "Set mode for: " << path << " (0x" << std::hex << current_mode << " -> 0x" << new_mode << std::dec << ")\n";
		}

		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}

int PS2McCommandLine::cmdEccCheck(const std::vector<std::string>& args)
{
	(void)args;
	if (!memoryCard)
	{
		Logger::error("No memory card open");
		return 1;
	}

	try
	{
		bool hasEcc = memoryCard->hasEcc();

		std::cout << "Memory card: " << currentMemCardPath << "\n";
		std::cout << "ECC status: " << (hasEcc ? "Enabled" : "Disabled") << "\n";

		if (hasEcc)
		{
			std::cout << "Running file system check with ECC validation...\n";
			bool result = memoryCard->check();
			if (result)
			{
				std::cout << "No errors found.\n";
				return 0;
			}
			else
			{
				std::cout << "Errors detected in file system.\n";
				return 1;
			}
		}
		else
		{
			std::cout << "Note: This card has no ECC data to validate.\n";
			std::cout << "Use 'check' command to verify file system integrity.\n";
		}

		return 0;
	}
	catch (const std::exception& e)
	{
		Logger::error("Error: {}", e.what());
		return 1;
	}
}
