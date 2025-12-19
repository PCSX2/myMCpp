// SPDX-FileCopyrightText: 2025 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <string>
#include <vector>
#include <memory>

class PS2MemoryCard;

class PS2McCommandLine
{
public:
	PS2McCommandLine();
	~PS2McCommandLine();

	int execute(int argc, char* argv[]);

private:
	void printHelp();
	void printVersion();

	// Commands
	int cmdLs(const std::vector<std::string>& args);
	int cmdDir(const std::vector<std::string>& args);
	int cmdExtract(const std::vector<std::string>& args);
	int cmdAdd(const std::vector<std::string>& args);
	int cmdImport(const std::vector<std::string>& args);
	int cmdExport(const std::vector<std::string>& args);
	int cmdDelete(const std::vector<std::string>& args);
	int cmdRemove(const std::vector<std::string>& args);
	int cmdMkdir(const std::vector<std::string>& args);
	int cmdFormat(const std::vector<std::string>& args);
	int cmdCheck(const std::vector<std::string>& args);
	int cmdDf(const std::vector<std::string>& args);
	int cmdClear(const std::vector<std::string>& args);
	int cmdSet(const std::vector<std::string>& args);
	int cmdEccCheck(const std::vector<std::string>& args);

	std::unique_ptr<PS2MemoryCard> memoryCard;
	bool ignoreEcc = false;
	bool noEcc = false;
	std::string currentMemCardPath;
};
