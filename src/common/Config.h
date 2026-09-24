// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

enum class RendererType
{
	Automatic,
	Vulkan,
	OpenGL,
	Metal
};

enum class LightingMode
{
	Off,
	Icon,
	Alternate1,
	Alternate2
};

enum class CameraMode
{
	Default,
	Flat,
	Near,
	High
};

struct Config
{
	struct GraphicsOptions
	{
		RendererType Renderer = RendererType::Automatic;
		std::string Adapter;
		bool AnimateIcons = true;
		LightingMode Lighting = LightingMode::Icon;
		CameraMode Camera = CameraMode::Default;
		bool VSync = true;
	} Graphics;

	struct UIOptions
	{
		std::string Theme = "dark";
		std::string Language = "en";
		bool AsciiMode = false;
		bool ToolbarLocked = false;
	} UI;

	struct BehaviorOptions
	{
		bool WarnOnDelete = true;
		bool ForceImport = false;
		bool DiscordRPCEnabled = true;
	} Behavior;

	struct PathOptions
	{
		std::string MemoryCardFolder;
		std::string ImportExportFolder;
	} Paths;

	struct PerformanceOptions
	{
		int MaxFPS = 30;
	} Performance;

	bool initialize(const fs::path& config_path = "myMCpp.ini");
	bool load(const fs::path& config_path);
	bool save() const;
	bool saveAs(const fs::path& config_path) const;

	fs::path ConfigPath;
	fs::path ResourcesPath;
};
