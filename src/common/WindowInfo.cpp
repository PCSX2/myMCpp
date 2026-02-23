// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "WindowInfo.h"
#include "Logger.h"
#include <vector>

#if defined(_WIN32)

#include <windows.h>
#include <dwmapi.h>

#ifdef _MSC_VER
#pragma comment(lib, "dwmapi.lib")
#endif

using DisplayConfigPathInfoArray = std::vector<DISPLAYCONFIG_PATH_INFO>;
using DisplayConfigModeInfoArray = std::vector<DISPLAYCONFIG_MODE_INFO>;

static std::string GetWin32ErrorString(DWORD error)
{
	if (error == 0)
		return "No error";

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

	std::string message(messageBuffer, size);
	LocalFree(messageBuffer);
	return message;
}

static std::optional<float> GetRefreshRateFromDisplayConfig(HWND hwnd)
{
	// Partially based on Chromium ui/display/win/display_config_helper.cc.
	const HMONITOR monitor = MonitorFromWindow(hwnd, 0);
	if (!monitor)
	{
		Logger::error("MonitorFromWindow() failed: {}", GetWin32ErrorString(GetLastError()));
		return std::nullopt;
	}

	MONITORINFOEXW mi = {};
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfoW(monitor, &mi))
	{
		Logger::error("GetMonitorInfoW() failed: {}", GetWin32ErrorString(GetLastError()));
		return std::nullopt;
	}

	std::vector<DISPLAYCONFIG_PATH_INFO> path_info;
	std::vector<DISPLAYCONFIG_MODE_INFO> mode_info;

	// I guess this could fail if it changes inbetween two calls... unlikely.
	for (;;)
	{
		UINT32 path_size = 0, mode_size = 0;
		LONG res = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &path_size, &mode_size);
		if (res != ERROR_SUCCESS)
		{
			Logger::error("GetDisplayConfigBufferSizes() failed: {}", GetWin32ErrorString(res));
			return std::nullopt;
		}

		path_info.resize(path_size);
		mode_info.resize(mode_size);
		res = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &path_size, path_info.data(), &mode_size, mode_info.data(), nullptr);
		if (res == ERROR_SUCCESS)
			break;
		if (res != ERROR_INSUFFICIENT_BUFFER)
		{
			Logger::error("QueryDisplayConfig() failed: {}", GetWin32ErrorString(res));
			return std::nullopt;
		}
	}

	for (const DISPLAYCONFIG_PATH_INFO& pi : path_info)
	{
		DISPLAYCONFIG_SOURCE_DEVICE_NAME sdn{};
		sdn.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
		sdn.header.size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);
		sdn.header.adapterId = pi.sourceInfo.adapterId;
		sdn.header.id = pi.sourceInfo.id;

		LONG res = DisplayConfigGetDeviceInfo(&sdn.header);
		if (res != ERROR_SUCCESS)
		{
			Logger::error("DisplayConfigGetDeviceInfo() failed: {}", GetWin32ErrorString(res));
			continue;
		}

		if (std::wcscmp(sdn.viewGdiDeviceName, mi.szDevice) == 0)
		{
			return static_cast<float>(static_cast<double>(pi.targetInfo.refreshRate.Numerator) /
									  static_cast<double>(pi.targetInfo.refreshRate.Denominator));
		}
	}

	return std::nullopt;
}

static std::optional<float> GetRefreshRateFromDWM([[maybe_unused]] HWND hwnd)
{
	BOOL composition_enabled;
	if (FAILED(DwmIsCompositionEnabled(&composition_enabled)))
		return std::nullopt;

	DWM_TIMING_INFO ti = {};
	ti.cbSize = sizeof(ti);
	HRESULT hr = DwmGetCompositionTimingInfo(nullptr, &ti);
	if (SUCCEEDED(hr))
	{
		if (ti.rateRefresh.uiNumerator == 0 || ti.rateRefresh.uiDenominator == 0)
			return std::nullopt;

		return static_cast<float>(ti.rateRefresh.uiNumerator) / static_cast<float>(ti.rateRefresh.uiDenominator);
	}

	return std::nullopt;
}

static std::optional<float> GetRefreshRateFromMonitor(HWND hwnd)
{
	HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	if (!mon)
		return std::nullopt;

	MONITORINFOEXW mi = {};
	mi.cbSize = sizeof(mi);
	if (GetMonitorInfoW(mon, &mi))
	{
		DEVMODEW dm = {};
		dm.dmSize = sizeof(dm);

		if (EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm) && dm.dmDisplayFrequency > 1)
			return static_cast<float>(dm.dmDisplayFrequency);
	}

	return std::nullopt;
}

std::optional<float> WindowInfo::QueryRefreshRateForWindow(const WindowInfo& wi)
{
	std::optional<float> ret;
	if (wi.type != Type::Win32 || !wi.window_handle)
		return ret;

	// Try DWM first, then fall back to integer values.
	const HWND hwnd = static_cast<HWND>(wi.window_handle);
	ret = GetRefreshRateFromDisplayConfig(hwnd);
	if (!ret.has_value())
	{
		ret = GetRefreshRateFromDWM(hwnd);
		if (!ret.has_value())
			ret = GetRefreshRateFromMonitor(hwnd);
	}

	return ret;
}

#elif defined(__APPLE__)

// Placeholder
std::optional<float> WindowInfo::QueryRefreshRateForWindow(const WindowInfo& wi)
{
	return std::nullopt;
}

#else

// Linux/X11
#if defined(__linux__)

#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>

template <typename F>
struct ScopedGuard
{
	F f;
	ScopedGuard(F f) : f(f) {}
	~ScopedGuard() { f(); }
};

static std::optional<float> GetRefreshRateFromXRandR(const WindowInfo& wi)
{
	// Assuming wi.display_connection is a Display* and window_handle is a Window
	Display* display = static_cast<Display*>(wi.display_connection);
	Window window = static_cast<Window>(reinterpret_cast<uintptr_t>(wi.window_handle));
	
	if (!display || !window)
		return std::nullopt;

	XRRScreenResources* res = XRRGetScreenResources(display, window);
	if (!res)
	{
		Logger::error("(GetRefreshRateFromXRandR) XRRGetScreenResources() failed");
		return std::nullopt;
	}
	ScopedGuard res_guard([res]() { XRRFreeScreenResources(res); });

	int num_monitors;
	XRRMonitorInfo* mi = XRRGetMonitors(display, window, True, &num_monitors);
	if (num_monitors < 0)
	{
		Logger::error("(GetRefreshRateFromXRandR) XRRGetMonitors() failed");
		return std::nullopt;
	}
	else if (num_monitors > 1)
	{
		Logger::warn("(GetRefreshRateFromXRandR) XRRGetMonitors() returned {} monitors, using first", num_monitors);
	}
	ScopedGuard mi_guard([mi]() { XRRFreeMonitors(mi); });

	if (mi->noutput <= 0)
	{
		Logger::error("(GetRefreshRateFromXRandR) Monitor has no outputs");
		return std::nullopt;
	}
	else if (mi->noutput > 1)
	{
		Logger::warn("(GetRefreshRateFromXRandR) Monitor has {} outputs, using first", mi->noutput);
	}

	XRROutputInfo* oi = XRRGetOutputInfo(display, res, mi->outputs[0]);
	if (!oi)
	{
		Logger::error("(GetRefreshRateFromXRandR) XRRGetOutputInfo() failed");
		return std::nullopt;
	}
	ScopedGuard oi_guard([oi]() { XRRFreeOutputInfo(oi); });

	XRRCrtcInfo* ci = XRRGetCrtcInfo(display, res, oi->crtc);
	if (!ci)
	{
		Logger::error("(GetRefreshRateFromXRandR) XRRGetCrtcInfo() failed");
		return std::nullopt;
	}
	ScopedGuard ci_guard([ci]() { XRRFreeCrtcInfo(ci); });

	XRRModeInfo* mode = nullptr;
	for (int i = 0; i < res->nmode; i++)
	{
		if (res->modes[i].id == ci->mode)
		{
			mode = &res->modes[i];
			break;
		}
	}
	if (!mode)
	{
		Logger::error("(GetRefreshRateFromXRandR) Failed to look up mode {} (of {})", static_cast<int>(ci->mode), res->nmode);
		return std::nullopt;
	}

	if (mode->dotClock == 0 || mode->hTotal == 0 || mode->vTotal == 0)
	{
		Logger::error("(GetRefreshRateFromXRandR) Modeline is invalid: {}/{}/{}", mode->dotClock, mode->hTotal, mode->vTotal);
		return std::nullopt;
	}

	return static_cast<float>(
		static_cast<double>(mode->dotClock) / (static_cast<double>(mode->hTotal) * static_cast<double>(mode->vTotal)));
}

#endif // __linux__

std::optional<float> WindowInfo::QueryRefreshRateForWindow(const WindowInfo& wi)
{
#if defined(__linux__)
	if (wi.type == WindowInfo::Type::X11)
		return GetRefreshRateFromXRandR(wi);
#endif
	return std::nullopt;
}

#endif
