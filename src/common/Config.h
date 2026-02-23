// SPDX-FileCopyrightText: 2025-2026 SternXD
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

class Config
{
public:
	Config();
	~Config() = default;

	bool initialize(const fs::path& config_path = "config.json");
	bool load(const fs::path& config_path);

	bool save() const;
	bool saveAs(const fs::path& config_path) const;

	std::string getRenderer() const;
	void setRenderer(const std::string& renderer);

	bool getAnimateIcons() const;
	void setAnimateIcons(bool enabled);
	std::string getLightingMode() const;
	void setLightingMode(const std::string& mode);
	std::string getCameraMode() const;
	void setCameraMode(const std::string& mode);

	int getWindowWidth() const;
	int getWindowHeight() const;
	void setWindowSize(int width, int height);

	bool getWindowFullscreen() const;
	void setWindowFullscreen(bool fullscreen);

	bool getWindowResizable() const;
	void setWindowResizable(bool resizable);

	int getSwapInterval() const;
	void setSwapInterval(int interval);

	bool getVSync() const;
	void setVSync(bool enabled);

	int getAntialiasing() const;
	void setAntialiasing(int samples);

	std::string getTheme() const;
	void setTheme(const std::string& theme);

	int getThumbnailSize() const;
	void setThumbnailSize(int size);

	std::string getLanguage() const;
	void setLanguage(const std::string& lang);

	bool getWarnOnDelete() const;
	void setWarnOnDelete(bool enabled);


	bool getHideToTrayOnClose() const;
	void setHideToTrayOnClose(bool enabled);

	bool getAsciiMode() const;
	void setAsciiMode(bool enabled);

	bool getForceImport() const;
	void setForceImport(bool enabled);

	std::string getMemoryCardFolder() const;
	void setMemoryCardFolder(const std::string& path);

	std::string getImportExportFolder() const;
	void setImportExportFolder(const std::string& path);

	int getMaxFPS() const;
	void setMaxFPS(int fps);

	bool getThreadedLoading() const;
	void setThreadedLoading(bool enabled);

	bool getDebugLogging() const;
	void setDebugLogging(bool enabled);

	bool getVerboseLogging() const;
	void setVerboseLogging(bool enabled);

	std::vector<std::string> getRecentFiles() const;
	void addRecentFile(const std::string& path);
	void clearRecentFiles();

	const json& getJson() const { return m_config; }
	json& getJson() { return m_config; }

	const fs::path& getConfigPath() const { return m_config_path; }
	const fs::path& getResourcesPath() const { return m_resources_path; }
	void setResourcesPath(const fs::path& path) { m_resources_path = path; }

private:
	json m_config;
	fs::path m_config_path;
	fs::path m_resources_path;

	void createDefaults();
	void ensureKeys();
};
