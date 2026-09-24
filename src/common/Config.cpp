// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "Config.h"
#include "Logger.h"

#include <SimpleIni.h>
#include <fstream>

bool Config::initialize(const fs::path& config_path)
{
	Logger::info("Loading config from: {}", config_path.string());
	ConfigPath = config_path;

	if (!fs::exists(config_path))
	{
		Logger::info("Config not found, creating defaults...");
		return save();
	}

	return load(config_path);
}

bool Config::load(const fs::path& config_path)
{
	std::ifstream config_file(config_path, std::ios::binary);
	if (!config_file.is_open())
	{
		Logger::error("Config: Failed to open config file: {}", config_path.string());
		return false;
	}

	const std::string data((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
	CSimpleIniA ini(true);
	if (ini.LoadData(data) != SI_OK)
	{
		Logger::error("Config: Failed to parse config file: {}", config_path.string());
		return false;
	}

	const Config defaults;

	// [Graphics]
	const std::string renderer = ini.GetValue("Graphics", "Renderer", "Automatic");
	if (renderer == "Vulkan")
		Graphics.Renderer = RendererType::Vulkan;
	else if (renderer == "OpenGL")
		Graphics.Renderer = RendererType::OpenGL;
	else if (renderer == "Metal")
		Graphics.Renderer = RendererType::Metal;
	else
		Graphics.Renderer = RendererType::Automatic;

	Graphics.Adapter = ini.GetValue("Graphics", "Adapter", defaults.Graphics.Adapter.c_str());
	Graphics.AnimateIcons = ini.GetBoolValue("Graphics", "AnimateIcons", defaults.Graphics.AnimateIcons);

	const std::string lighting = ini.GetValue("Graphics", "LightingMode", "Icon");
	if (lighting == "Off")
		Graphics.Lighting = LightingMode::Off;
	else if (lighting == "Alternate1")
		Graphics.Lighting = LightingMode::Alternate1;
	else if (lighting == "Alternate2")
		Graphics.Lighting = LightingMode::Alternate2;
	else
		Graphics.Lighting = LightingMode::Icon;

	const std::string camera = ini.GetValue("Graphics", "CameraMode", "Default");
	if (camera == "Flat")
		Graphics.Camera = CameraMode::Flat;
	else if (camera == "Near")
		Graphics.Camera = CameraMode::Near;
	else if (camera == "High")
		Graphics.Camera = CameraMode::High;
	else
		Graphics.Camera = CameraMode::Default;

	Graphics.VSync = ini.GetBoolValue("Graphics", "VSync", defaults.Graphics.VSync);

	// [UI]
	UI.Theme = ini.GetValue("UI", "Theme", defaults.UI.Theme.c_str());
	UI.Language = ini.GetValue("UI", "Language", defaults.UI.Language.c_str());
	UI.AsciiMode = ini.GetBoolValue("UI", "AsciiMode", defaults.UI.AsciiMode);
	UI.ToolbarLocked = ini.GetBoolValue("UI", "ToolbarLocked", defaults.UI.ToolbarLocked);

	// [Behavior]
	Behavior.WarnOnDelete = ini.GetBoolValue("Behavior", "WarnOnDelete", defaults.Behavior.WarnOnDelete);
	Behavior.ForceImport = ini.GetBoolValue("Behavior", "ForceImport", defaults.Behavior.ForceImport);
	Behavior.DiscordRPCEnabled = ini.GetBoolValue("Behavior", "DiscordRPCEnabled", defaults.Behavior.DiscordRPCEnabled);

	// [Paths]
	Paths.MemoryCardFolder = ini.GetValue("Paths", "MemoryCardFolder", defaults.Paths.MemoryCardFolder.c_str());
	Paths.ImportExportFolder = ini.GetValue("Paths", "ImportExportFolder", defaults.Paths.ImportExportFolder.c_str());

	// [Performance]
	Performance.MaxFPS = static_cast<int>(ini.GetLongValue("Performance", "MaxFPS", defaults.Performance.MaxFPS));
	return true;
}

bool Config::save() const
{
	Logger::info("Config: Saving to: {}", ConfigPath.string());
	return saveAs(ConfigPath);
}

bool Config::saveAs(const fs::path& config_path) const
{
	Logger::info("Config: Writing config to: {}", config_path.string());

	CSimpleIniA ini(true);

	// [Graphics]
	const char* renderer = "Automatic";
	switch (Graphics.Renderer)
	{
		case RendererType::Vulkan:
			renderer = "Vulkan";
			break;
		case RendererType::OpenGL:
			renderer = "OpenGL";
			break;
		case RendererType::Metal:
			renderer = "Metal";
			break;
		case RendererType::Automatic:
			break;
	}
	ini.SetValue("Graphics", "Renderer", renderer);
	ini.SetValue("Graphics", "Adapter", Graphics.Adapter.c_str());
	ini.SetBoolValue("Graphics", "AnimateIcons", Graphics.AnimateIcons);

	const char* lighting = "Icon";
	switch (Graphics.Lighting)
	{
		case LightingMode::Off:
			lighting = "Off";
			break;
		case LightingMode::Alternate1:
			lighting = "Alternate1";
			break;
		case LightingMode::Alternate2:
			lighting = "Alternate2";
			break;
		case LightingMode::Icon:
			break;
	}
	ini.SetValue("Graphics", "LightingMode", lighting);

	const char* camera = "Default";
	switch (Graphics.Camera)
	{
		case CameraMode::Flat:
			camera = "Flat";
			break;
		case CameraMode::Near:
			camera = "Near";
			break;
		case CameraMode::High:
			camera = "High";
			break;
		case CameraMode::Default:
			break;
	}
	ini.SetValue("Graphics", "CameraMode", camera);
	ini.SetBoolValue("Graphics", "VSync", Graphics.VSync);

	// [UI]
	ini.SetValue("UI", "Theme", UI.Theme.c_str());
	ini.SetValue("UI", "Language", UI.Language.c_str());
	ini.SetBoolValue("UI", "AsciiMode", UI.AsciiMode);
	ini.SetBoolValue("UI", "ToolbarLocked", UI.ToolbarLocked);

	// [Behavior]
	ini.SetBoolValue("Behavior", "WarnOnDelete", Behavior.WarnOnDelete);
	ini.SetBoolValue("Behavior", "ForceImport", Behavior.ForceImport);
	ini.SetBoolValue("Behavior", "DiscordRPCEnabled", Behavior.DiscordRPCEnabled);

	// [Paths]
	ini.SetValue("Paths", "MemoryCardFolder", Paths.MemoryCardFolder.c_str());
	ini.SetValue("Paths", "ImportExportFolder", Paths.ImportExportFolder.c_str());

	// [Performance]
	ini.SetLongValue("Performance", "MaxFPS", Performance.MaxFPS);

	if (config_path.has_parent_path())
	{
		std::error_code ec;
		fs::create_directories(config_path.parent_path(), ec);
	}

	std::string data;
	if (ini.Save(data) != SI_OK)
	{
		Logger::error("Config: Failed to serialize config");
		return false;
	}

	std::ofstream config_file(config_path, std::ios::binary);
	if (!config_file.is_open())
	{
		Logger::error("Config: Failed to open config file for writing: {}", config_path.string());
		return false;
	}

	config_file.write(data.data(), static_cast<std::streamsize>(data.size()));
	if (!config_file)
	{
		Logger::error("Config: Failed to write config file: {}", config_path.string());
		return false;
	}

	Logger::info("Config: Saved config to: {}", config_path.string());
	return true;
}
