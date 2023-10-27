#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

#include "Hy3Layout.hpp"
#include "log.hpp"

inline HANDLE PHANDLE = nullptr;
inline std::unique_ptr<Hy3Layout> g_Hy3Layout;

inline void errorNotif() {
	HyprlandAPI::addNotificationV2(
	    PHANDLE,
	    {
	        {"text", "Something has gone very wrong. Check the log for details."},
	        {"time", (uint64_t) 10000},
	        {"color", CColor(1.0, 0.0, 0.0, 1.0)},
	        {"icon", ICON_ERROR},
	    }
	);
}
