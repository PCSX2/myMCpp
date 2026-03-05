// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#include "Config.h"
#include <fstream>
#include "Logger.h"

Config::Config()
{
	createDefaults();
}

bool Config::initialize(const fs::path& config_path)
{
	Logger::info("Loading config from: {}", config_path.string());

	m_config_path = config_path;

	if (!fs::exists(config_path))
	{
		Logger::info("Config not found, creating defaults...");
		createDefaults();
		return saveAs(config_path);
	}

	return load(config_path);
}

bool Config::load(const fs::path& config_path)
{
	try
	{
		std::ifstream config_file(config_path);
		if (!config_file.is_open())
		{
			Logger::error("Config: Failed to open config file: {}", config_path.string());
			createDefaults();
			return false;
		}

		m_config = json::parse(config_file);
		m_config_path = config_path;
		ensureKeys();

		return true;
	}
	catch (const std::exception& e)
	{
		Logger::error("Config: Failed to parse config file: {}", e.what());
		createDefaults();
		return false;
	}
}

bool Config::save() const
{
	Logger::info("Config: Saving to: {}", m_config_path.string());
	return saveAs(m_config_path);
}

bool Config::saveAs(const fs::path& config_path) const
{
	try
	{
		Logger::info("Config: Writing config to: {}", config_path.string());

		// Make sure directory exists
		if (config_path.has_parent_path())
		{
			fs::create_directories(config_path.parent_path());
		}

		std::ofstream config_file(config_path);
		if (!config_file.is_open())
		{
			Logger::error("Config: Failed to open config file for writing: {}", config_path.string());
			return false;
		}

		std::string json_str = m_config.dump(4);
		config_file << json_str;
		config_file.close();

		Logger::info("Config: Saved config to: {}", config_path.string());
		return true;
	}
	catch (const std::exception& e)
	{
		Logger::error("Config: Failed to save config file: {}", e.what());
		return false;
	}
}

std::string Config::getRenderer() const
{
	try
	{
		return m_config["graphics"]["renderer"].get<std::string>();
	}
	catch (...)
	{
#if defined(__APPLE__)
		return "metal";
#else
		return "vulkan";
#endif
	}
}

void Config::setRenderer(const std::string& renderer)
{
	m_config["graphics"]["renderer"] = renderer;
}

bool Config::getAnimateIcons() const
{
	try
	{
		return m_config["graphics"]["animate_icons"].get<bool>();
	}
	catch (...)
	{
		return true;
	}
}

void Config::setAnimateIcons(bool enabled)
{
	m_config["graphics"]["animate_icons"] = enabled;
}

std::string Config::getLightingMode() const
{
	try
	{
		return m_config["graphics"]["lighting_mode"].get<std::string>();
	}
	catch (...)
	{
		return "icon";
	}
}

void Config::setLightingMode(const std::string& mode)
{
	m_config["graphics"]["lighting_mode"] = mode;
}

std::string Config::getCameraMode() const
{
	try
	{
		return m_config["graphics"]["camera_mode"].get<std::string>();
	}
	catch (...)
	{
		return "default";
	}
}

void Config::setCameraMode(const std::string& mode)
{
	m_config["graphics"]["camera_mode"] = mode;
}

int Config::getWindowWidth() const
{
	try
	{
		return m_config["window"]["width"].get<int>();
	}
	catch (...)
	{
		return 1280;
	}
}

int Config::getWindowHeight() const
{
	try
	{
		return m_config["window"]["height"].get<int>();
	}
	catch (...)
	{
		return 720;
	}
}

void Config::setWindowSize(int width, int height)
{
	m_config["window"]["width"] = width;
	m_config["window"]["height"] = height;
}

bool Config::getWindowFullscreen() const
{
	try
	{
		return m_config["window"]["fullscreen"].get<bool>();
	}
	catch (...)
	{
		return false;
	}
}

void Config::setWindowFullscreen(bool fullscreen)
{
	m_config["window"]["fullscreen"] = fullscreen;
}

bool Config::getWindowResizable() const
{
	try
	{
		return m_config["window"]["resizable"].get<bool>();
	}
	catch (...)
	{
		return true;
	}
}

void Config::setWindowResizable(bool resizable)
{
	m_config["window"]["resizable"] = resizable;
}

int Config::getSwapInterval() const
{
	try
	{
		return m_config["graphics"]["swap_interval"].get<int>();
	}
	catch (...)
	{
		return 1;
	}
}

void Config::setSwapInterval(int interval)
{
	m_config["graphics"]["swap_interval"] = interval;
}

bool Config::getVSync() const
{
	try
	{
		return m_config["graphics"]["vsync"].get<bool>();
	}
	catch (...)
	{
		return true;
	}
}

void Config::setVSync(bool enabled)
{
	m_config["graphics"]["vsync"] = enabled;
}

int Config::getAntialiasing() const
{
	try
	{
		return m_config["graphics"]["antialiasing"].get<int>();
	}
	catch (...)
	{
		return 4;
	}
}

void Config::setAntialiasing(int samples)
{
	m_config["graphics"]["antialiasing"] = samples;
}

std::string Config::getTheme() const
{
	try
	{
		return m_config["ui"]["theme"].get<std::string>();
	}
	catch (...)
	{
		return "dark";
	}
}

void Config::setTheme(const std::string& theme)
{
	m_config["ui"]["theme"] = theme;
}

int Config::getThumbnailSize() const
{
	try
	{
		return m_config["ui"]["thumbnail_size"].get<int>();
	}
	catch (...)
	{
		return 64;
	}
}

void Config::setThumbnailSize(int size)
{
	m_config["ui"]["thumbnail_size"] = size;
}

std::string Config::getLanguage() const
{
	try
	{
		return m_config["ui"]["language"].get<std::string>();
	}
	catch (...)
	{
		return "en";
	}
}

void Config::setLanguage(const std::string& lang)
{
	m_config["ui"]["language"] = lang;
}

bool Config::getWarnOnDelete() const
{
	try
	{
		return m_config["behavior"]["warn_on_delete"].get<bool>();
	}
	catch (...)
	{
		return true;
	}
}

void Config::setWarnOnDelete(bool enabled)
{
	m_config["behavior"]["warn_on_delete"] = enabled;
}

bool Config::getHideToTrayOnClose() const
{
	try
	{
		return m_config["behavior"]["hide_to_tray"].get<bool>();
	}
	catch (...)
	{
		return false;
	}
}

void Config::setHideToTrayOnClose(bool enabled)
{
	m_config["behavior"]["hide_to_tray"] = enabled;
}

bool Config::getAsciiMode() const
{
	try
	{
		return m_config["ui"]["ascii_mode"].get<bool>();
	}
	catch (...)
	{
		return false;
	}
}

void Config::setAsciiMode(bool enabled)
{
	m_config["ui"]["ascii_mode"] = enabled;
}

bool Config::getForceImport() const
{
	try
	{
		return m_config["behavior"]["force_import"].get<bool>();
	}
	catch (...)
	{
		return false;
	}
}

void Config::setForceImport(bool enabled)
{
	m_config["behavior"]["force_import"] = enabled;
}

bool Config::getDiscordRPCEnabled() const
{
	try
	{
		return m_config["behavior"]["discord_rpc_enabled"].get<bool>();
	}
	catch (...)
	{
		return false;
	}
}

void Config::setDiscordRPCEnabled(bool enabled)
{
	m_config["behavior"]["discord_rpc_enabled"] = enabled;
}

std::string Config::getMemoryCardFolder() const
{
	try
	{
		return m_config["paths"]["memory_card_folder"].get<std::string>();
	}
	catch (...)
	{
		return "./memory_cards";
	}
}

void Config::setMemoryCardFolder(const std::string& path)
{
	m_config["paths"]["memory_card_folder"] = path;
}

std::string Config::getImportExportFolder() const
{
	try
	{
		return m_config["paths"]["import_export_folder"].get<std::string>();
	}
	catch (...)
	{
		return "";
	}
}

void Config::setImportExportFolder(const std::string& path)
{
	m_config["paths"]["import_export_folder"] = path;
}

int Config::getMaxFPS() const
{
	try
	{
		return m_config["performance"]["max_fps"].get<int>();
	}
	catch (...)
	{
		return 30;
	}
}

void Config::setMaxFPS(int fps)
{
	m_config["performance"]["max_fps"] = fps;
}

bool Config::getThreadedLoading() const
{
	try
	{
		return m_config["performance"]["threaded_loading"].get<bool>();
	}
	catch (...)
	{
		return true;
	}
}

void Config::setThreadedLoading(bool enabled)
{
	m_config["performance"]["threaded_loading"] = enabled;
}

bool Config::getVerboseLogging() const
{
	try
	{
		return m_config["debug"]["verbose"].get<bool>();
	}
	catch (...)
	{
		return false;
	}
}

void Config::setVerboseLogging(bool enabled)
{
	m_config["debug"]["verbose"] = enabled;
}

bool Config::getDebugLogging() const
{
	try
	{
		return m_config["debug"]["logging"].get<bool>();
	}
	catch (...)
	{
		return false;
	}
}

void Config::setDebugLogging(bool enabled)
{
	m_config["debug"]["logging"] = enabled;
}

std::vector<std::string> Config::getRecentFiles() const
{
	std::vector<std::string> files;
	try
	{
		if (m_config.contains("recent_files") && m_config["recent_files"].is_array())
		{
			for (const auto& file : m_config["recent_files"])
			{
				files.push_back(file.get<std::string>());
			}
		}
	}
	catch (...)
	{
	}
	return files;
}

void Config::addRecentFile(const std::string& path)
{
	if (!m_config.contains("recent_files") || !m_config["recent_files"].is_array())
	{
		m_config["recent_files"] = json::array();
	}

	// Remove if already exists (to move it to front)
	auto& recent = m_config["recent_files"];
	for (auto it = recent.begin(); it != recent.end(); ++it)
	{
		if (it->get<std::string>() == path)
		{
			recent.erase(it);
			break;
		}
	}

	// Add to front
	recent.insert(recent.begin(), path);

	// Keep only last 10
	while (recent.size() > 10)
	{
		recent.erase(recent.end() - 1);
	}
}

void Config::clearRecentFiles()
{
	m_config["recent_files"] = json::array();
}

void Config::createDefaults()
{
	m_config = {
		{"graphics", {{"renderer", "vulkan"},
						 {"vsync", true},
						 {"swap_interval", 1},
						 {"antialiasing", 4},
						 {"animate_icons", true},
						 {"lighting_mode", "icon"},
						 {"camera_mode", "default"}}},
		{"window", {{"width", 1280},
					   {"height", 720},
					   {"fullscreen", false},
					   {"resizable", true}}},
		{"ui", {{"theme", "dark"},
				   {"thumbnail_size", 64},
				   {"ascii_mode", false},
				   {"language", "en"}}},
		{"behavior", {{"warn_on_delete", true},
						 {"hide_to_tray", false},
						 {"force_import", false},
						 {"discord_rpc_enabled", true}}},
		{"paths", {{"memory_card_folder", "./memory_cards"}}},
		{"debug", {{"logging", false},
					  {"verbose", false}}},
		{"performance", {{"max_fps", 30},
							{"threaded_loading", true}}}};
}

void Config::ensureKeys()
{
	if (!m_config.contains("graphics"))
		m_config["graphics"] = json::object();
	if (!m_config.contains("window"))
		m_config["window"] = json::object();
	if (!m_config.contains("ui"))
		m_config["ui"] = json::object();
	if (!m_config.contains("behavior"))
		m_config["behavior"] = json::object();
	if (!m_config.contains("paths"))
		m_config["paths"] = json::object();
	if (!m_config.contains("debug"))
		m_config["debug"] = json::object();
	if (!m_config.contains("performance"))
		m_config["performance"] = json::object();

	if (!m_config["graphics"].contains("renderer"))
		m_config["graphics"]["renderer"] = "vulkan";
	if (!m_config["graphics"].contains("vsync"))
		m_config["graphics"]["vsync"] = true;
	if (!m_config["graphics"].contains("swap_interval"))
		m_config["graphics"]["swap_interval"] = 1;
	if (!m_config["graphics"].contains("antialiasing"))
		m_config["graphics"]["antialiasing"] = 4;
	if (!m_config["graphics"].contains("animate_icons"))
		m_config["graphics"]["animate_icons"] = true;
	if (!m_config["graphics"].contains("lighting_mode"))
		m_config["graphics"]["lighting_mode"] = "icon";
	if (!m_config["graphics"].contains("camera_mode"))
		m_config["graphics"]["camera_mode"] = "default";

	if (!m_config["window"].contains("width"))
		m_config["window"]["width"] = 1280;
	if (!m_config["window"].contains("height"))
		m_config["window"]["height"] = 720;
	if (!m_config["window"].contains("fullscreen"))
		m_config["window"]["fullscreen"] = false;
	if (!m_config["window"].contains("resizable"))
		m_config["window"]["resizable"] = true;

	if (!m_config["ui"].contains("theme"))
		m_config["ui"]["theme"] = "dark";
	if (!m_config["ui"].contains("thumbnail_size"))
		m_config["ui"]["thumbnail_size"] = 64;
	if (!m_config["ui"].contains("ascii_mode"))
		m_config["ui"]["ascii_mode"] = false;
	if (!m_config["ui"].contains("language"))
		m_config["ui"]["language"] = "en";

	if (!m_config["behavior"].contains("warn_on_delete"))
		m_config["behavior"]["warn_on_delete"] = true;
	if (!m_config["behavior"].contains("hide_to_tray"))
		m_config["behavior"]["hide_to_tray"] = false;
	if (!m_config["behavior"].contains("force_import"))
		m_config["behavior"]["force_import"] = false;
	if (!m_config["behavior"].contains("discord_rpc_enabled"))
		m_config["behavior"]["discord_rpc_enabled"] = true;

	if (!m_config["paths"].contains("memory_card_folder"))
		m_config["paths"]["memory_card_folder"] = "./memory_cards";

	if (!m_config["debug"].contains("logging"))
		m_config["debug"]["logging"] = false;
	if (!m_config["debug"].contains("verbose"))
		m_config["debug"]["verbose"] = false;

	if (!m_config["performance"].contains("max_fps"))
		m_config["performance"]["max_fps"] = 30;
	if (!m_config["performance"].contains("threaded_loading"))
		m_config["performance"]["threaded_loading"] = true;
}
